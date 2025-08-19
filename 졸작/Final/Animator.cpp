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
    mFrame = static_cast<int32_t>(mUpdateTime * framerate);
    mFrame = min(mFrame, animClip.frameCount - 1);
    mNextFrame = min(mFrame + 1, animClip.frameCount - 1);
    mFrameRatio = static_cast<float>(mUpdateTime * framerate - mFrame);
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
    mPrevFrame = static_cast<int32_t>(mPrevUpdateTime * framerate);
    mPrevFrame = min(mPrevFrame, animClip.frameCount - 1);
    mPrevNextFrame = min(mPrevFrame + 1, animClip.frameCount - 1);
    mPrevFrameRatio = static_cast<float>(mPrevUpdateTime * framerate - mPrevFrame);
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

    core.GetAnimationCB()->CopyData(&animData, sizeof(AnimationConstants));

    cmdList->SetPipelineState(core.GetShader()->GetComputePSO());
    cmdList->SetComputeRootSignature(core.GetRootSig()->Get());
    cmdList->SetComputeRootConstantBufferView(2, core.GetAnimationCB()->GetGPUVirtualAddress());

    cmdList->SetComputeRootShaderResourceView(5, GetBoneFrameBuffer()->GetGPUVirtualAddress());  // t1, space0
    cmdList->SetComputeRootShaderResourceView(6, GetOffsetBuffer()->GetGPUVirtualAddress());     // t2, space0

    cmdList->SetComputeRootUnorderedAccessView(7, GetFinalBuffer()->GetGPUVirtualAddress());     // u0

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
