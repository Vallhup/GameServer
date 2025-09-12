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
        blendTime += deltaTime;
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

    mUpdateTime += deltaTime;
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

    mPrevUpdateTime += deltaTime;

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

    // 버퍼 생성
    mBoneFrameBuffer = make_unique<UploadBuffer>();
    mOffsetBuffer = make_unique<UploadBuffer>();
    mFinalBuffer = make_unique<UploadBuffer>();
    mAnimationCB = make_unique<UploadBuffer>();

    // BoneFrame 버퍼 - 레퍼런스와 동일한 구조
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

    // 모든 애니메이션 데이터를 하나의 버퍼에 복사
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

    //DebugAnimationInfo();

    AnimationConstants animData = {};
    animData.boneCount = GetBoneCount();
    animData.currentFrame = GetCurrentFrame();
    animData.nextFrame = GetNextFrame();
    animData.ratio = GetFrameRatio();
    animData.animationOffset = GetCurrentAnimOffset();
    animData.isBlending = IsBlending();
    animData.prevCurrentFrame = GetPrevCurrentFrame();
    animData.prevNextFrame = GetPrevNextFrame();
    animData.prevRatio = GetPrevFrameRatio();
    animData.prevAnimationOffset = GetPrevAnimOffset();
    animData.blendRatio = GetBlendRatio();

    mAnimationCB->CopyData(&animData, sizeof(AnimationConstants));

    cmdList->SetPipelineState(core.GetShader()->GetComputePSO());
    cmdList->SetComputeRootSignature(core.GetRootSig()->Get());
    cmdList->SetComputeRootConstantBufferView(2, mAnimationCB->GetGPUVirtualAddress());

    cmdList->SetComputeRootShaderResourceView(7, GetBoneFrameBuffer()->GetGPUVirtualAddress());  // 레지 넘버링 부분
    cmdList->SetComputeRootShaderResourceView(8, GetOffsetBuffer()->GetGPUVirtualAddress());     // 레지 넘버링 부분

    cmdList->SetComputeRootUnorderedAccessView(10, GetFinalBuffer()->GetGPUVirtualAddress());     // 레지 넘버링 부분

    UINT groupCount = (animData.boneCount + 255) / 256;  // 256으로 나눠서 올림
    cmdList->Dispatch(groupCount, 1, 1);
}

void Animator::LoadAnimationFromImporter(DX12Core& core, const Importer& importer)
{
    const auto& animations = importer.GetAnimations();
    const auto& skeleton = importer.GetSkeleton();

    if (!animations.empty()) {
        SetAnimationData(core, animations);
        SetSkeletonData(skeleton);
        OutputDebugStringA("Animation data loaded!\n");
    }
}

void Animator::DebugAnimationInfo()
{
    OutputDebugStringA(("Bone Count: " + to_string(GetBoneCount()) + "\n").c_str());
    OutputDebugStringA(("Current Frame: " + to_string(GetCurrentFrame()) + "\n").c_str());
    OutputDebugStringA(("Frame Ratio: " + to_string(GetFrameRatio()) + "\n").c_str());
}
