#include "pch.h"
#include "Animator.h"
#include "DX12Core.h"
#include "GameObject.h"
#include "Shader.h"
#include "RootSignature.h"

void Animator::Update(float deltaTime)
{
    if (mAnimations.empty()) return;

    if (mIsBlending) {
        blendTime += deltaTime * animationSpeed;
        blendRatio = blendTime / blendDuration;

        if (blendRatio >= 1.0f) {
            blendRatio = 1.0f;
            mIsBlending = false;
            mPrevClipIndex = -1;
        }

        UpdatePrevAnimation(deltaTime);
        UpdateCurrentAnimation(deltaTime);
    }
    else
        UpdateCurrentAnimation(deltaTime);
}

void Animator::UpdateCurrentAnimation(float deltaTime)
{
    mCurrentAnimOffset = 0;

    for (int i = 0; i < mClipIndex; ++i) {
        mCurrentAnimOffset += mAnimations[i].keyFrames.size();
    }

    mUpdateTime += deltaTime * animationSpeed;
    const auto& animClip = mAnimations[mClipIndex];

    if (mUpdateTime >= animClip.duration) {
        mUpdateTime = 0.0f;
    }

    const float framerate = static_cast<float>(animClip.frameCount) / animClip.duration;
    float frameFloat = (mUpdateTime * framerate) + 1.0f;

    mFrame = static_cast<int32_t>(frameFloat);
    mFrame = max(1, min(mFrame, animClip.frameCount));

    if (mFrame == animClip.frameCount) {
        mNextFrame = 1;
    }
    else {
        mNextFrame = mFrame + 1;
    }

    mFrameRatio = frameFloat - mFrame;
}

void Animator::UpdatePrevAnimation(float deltaTime)
{
    if (mPrevClipIndex < 0 || mPrevClipIndex >= mAnimations.size()) return;

    mPrevAnimOffset = 0;

    for (int i = 0; i < mPrevClipIndex; ++i) {
        mPrevAnimOffset += mAnimations[i].keyFrames.size();
    }

    mPrevUpdateTime += deltaTime * animationSpeed;

    const auto& animClip = mAnimations[mPrevClipIndex];

    if (mPrevUpdateTime >= animClip.duration) {
        mPrevUpdateTime = 0.0f;
    }

    const float framerate = static_cast<float>(animClip.frameCount) / animClip.duration;

    float frameFloat = (mPrevUpdateTime * framerate) + 1.0f;
    mPrevFrame = static_cast<int32_t>(frameFloat);
    mPrevFrame = max(1, min(mPrevFrame, animClip.frameCount));

    if (mPrevFrame == animClip.frameCount) {
        mPrevNextFrame = 1;
    }
    else {
        mPrevNextFrame = mPrevFrame + 1;
    }

    mPrevFrameRatio = frameFloat - mPrevFrame;
}

void Animator::SetAnimationData(DX12Core& core, const vector<AnimClipInfo>& animations)
{
    mAnimations = animations;
    if (!mAnimations.empty()) {
        mBoneCount = static_cast<int>(mAnimations[0].keyFrames.size() / mAnimations[0].frameCount);
        CreateBuffers(core);
    }
}

void Animator::SetSkeletonData(const SkeletonData& skeleton)
{
    mBones = skeleton.bones;

    if (mOffsetBuffer && !mBones.empty()) {
        vector<XMMATRIX> offsetMatrices;
        for (const auto& bone : mBones) {
            offsetMatrices.push_back(bone.matOffset);
        }
        mOffsetBuffer->CopyData(offsetMatrices.data(), offsetMatrices.size() * sizeof(XMMATRIX));
    }
}

void Animator::CreateBuffers(DX12Core& core)
{
    if (mAnimations.empty()) return;

    // ���� ����
    mBoneFrameBuffer = make_unique<UploadBuffer>();
    mOffsetBuffer = make_unique<UploadBuffer>();
    mFinalBuffer = make_unique<UAVBuffer>();
    mAnimationCB = make_unique<UploadBuffer>();

    // BoneFrame ���� - ���۷����� ������ ����
    size_t totalKeyFrames = 0;
    for (const auto& anim : mAnimations) {
        totalKeyFrames += anim.keyFrames.size();
    }

    mBoneFrameBuffer->Initialize(
        core.GetDevice(),
        totalKeyFrames * sizeof(AnimFrameParams)
    );

    mOffsetBuffer->Initialize(
        core.GetDevice(),
        mBoneCount * sizeof(XMMATRIX)
    );

    mFinalBuffer->Initialize(
        core.GetDevice(),
        mBoneCount * sizeof(XMMATRIX)
    );

    mAnimationCB->Initialize(
        core.GetDevice(),
        sizeof(AnimationConstants)
    );

    vector<AnimFrameParams> allFrameData;
    for (const auto& anim : mAnimations) {
        allFrameData.insert(allFrameData.end(), anim.keyFrames.begin(), anim.keyFrames.end());
    }

    mBoneFrameBuffer->CopyData(allFrameData.data(), allFrameData.size() * sizeof(AnimFrameParams));
    mIsInitialized = true;
}

void Animator::PlayAnimation(int animIndex)
{
    if (animIndex < 0 || animIndex >= mAnimations.size()) return;

    mClipIndex = animIndex;
    mUpdateTime = 0.0f;
}

void Animator::TransitionToAnimation(int animIndex, float Duration)
{
    if (animIndex < 0 || animIndex >= mAnimations.size()) return;
    if (mClipIndex == animIndex) return;

    mPrevClipIndex = mClipIndex;
    mClipIndex = animIndex;
    blendDuration = Duration;
    mPrevUpdateTime = mUpdateTime;
    mUpdateTime = 0.0f;
    mIsBlending = true;

    blendTime = 0.0f;
    blendRatio = 0.0f;
}

void Animator::ExecuteComputeShader(DX12Core& core)
{
    auto cmdList = core.GetGraphicsCmdList();

    AnimationConstants animData = {};
    animData.boneCount = mBoneCount;
    animData.currentFrame = mFrame;
    animData.nextFrame = mNextFrame;
    animData.ratio = mFrameRatio;
    animData.animationOffset = mCurrentAnimOffset;
    animData.isBlending = mIsBlending;
    animData.prevCurrentFrame = mPrevFrame;
    animData.prevNextFrame = mPrevNextFrame;
    animData.prevRatio = mPrevFrameRatio;
    animData.prevAnimationOffset = mPrevAnimOffset;
    animData.blendRatio = blendRatio;

    mAnimationCB->CopyData(&animData, sizeof(AnimationConstants));

    cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::Compute));
    cmdList->SetComputeRootSignature(core.GetRootSig()->Get());
    cmdList->SetComputeRootConstantBufferView(2, mAnimationCB->GetGPUVirtualAddress());

    cmdList->SetComputeRootShaderResourceView(8, GetBoneFrameBuffer()->GetGPUVirtualAddress());  
    cmdList->SetComputeRootShaderResourceView(9, GetOffsetBuffer()->GetGPUVirtualAddress());     

    cmdList->SetComputeRootUnorderedAccessView(11, GetFinalBuffer()->GetGPUVirtualAddress());     

    UINT groupCount = (animData.boneCount + 255) / 256;  
    cmdList->Dispatch(groupCount, 1, 1);
}

void Animator::LoadAnimationFromImporter(DX12Core& core, const Importer& importer)
{
    const auto& animations = importer.GetAnimations();
    const auto& skeleton = importer.GetSkeleton();

    if (!animations.empty()) {
        SetAnimationData(core, animations);
        SetSkeletonData(skeleton);

        // later if I want to debug anim info - turn on
        //DebugAnimationInfo();
        OutputDebugStringA("Animation data loaded!\n");
    }
}

void Animator::DebugAnimationInfo()
{
    OutputDebugStringA(("Bone Count: " + to_string(mBoneCount) + "\n").c_str());
    OutputDebugStringA(("Current Frame: " + to_string(mFrame) + "\n").c_str());
    OutputDebugStringA(("Frame Ratio: " + to_string(mFrameRatio) + "\n").c_str());

    for (int i = 0; i < mAnimations.size(); ++i) {
        OutputDebugStringA(("[" + to_string(i) + "] " + mAnimations[i].animName + "\n").c_str());
    }
}

float Animator::GetAnimationProgress() const
{
    if (mAnimations.empty() || mClipIndex < 0 || mClipIndex >= mAnimations.size())
        return 0.0f;

    const auto& clip = mAnimations[mClipIndex];
    if (clip.duration <= 0.0f)
        return 0.0f;

    return mUpdateTime / clip.duration;     // 0.0 ~ 1.0
}