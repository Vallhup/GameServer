#include "pch.h"
#include "Animator.h"
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

void Animator::UpdateAnimationOffsets()
{
    mCurrentAnimOffset = 0;
    for (int i = 0; i < mClipIndex; ++i) {
        mCurrentAnimOffset += mAnimations[i].keyFrames.size();
    }

    if (mIsBlending && mPrevClipIndex >= 0) {
        mPrevAnimOffset = 0;
        for (int i = 0; i < mPrevClipIndex; ++i) {
            mPrevAnimOffset += mAnimations[i].keyFrames.size();
        }
    }
}

void Animator::UpdateCurrentAnimation(float deltaTime)
{
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

    mPrevUpdateTime += deltaTime * animationSpeed;

    const auto& animClip = mAnimations[mPrevClipIndex];

    if (mPrevUpdateTime >= animClip.duration) {
        mPrevUpdateTime = animClip.duration - 0.001f;
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

int Animator::GetBoneIndex(const string& name)
{
    for (int i = 0; i < mBones.size(); ++i)
    {
        if (mBones[i].boneName == name)
            return i;
    }
    return -1;
}

XMVECTOR Animator::QuaternionNlerp(XMVECTOR Q1, XMVECTOR Q2, float t)
{
    float dotResult = XMVectorGetX(XMVector4Dot(Q1, Q2));
    if (dotResult < 0.0f)
        Q2 = XMVectorNegate(Q2);

    XMVECTOR result = XMVectorLerp(Q1, Q2, t);

    return XMVector4Normalize(result);
}

void Animator::GetInterpolatedSRT(int boneIndex, int clipIndex, int currentFrame, int nextFrame, float ratio,
    XMVECTOR& outS, XMVECTOR& outR, XMVECTOR& outT)
{
    int currIdx = (currentFrame - 1) * mBoneCount + boneIndex;
    int nextIdx = (nextFrame - 1) * mBoneCount + boneIndex;

    auto& keyFrames = mAnimations[clipIndex].keyFrames;

    XMVECTOR currS = XMLoadFloat4(&keyFrames[currIdx].scale);
    XMVECTOR currR = XMLoadFloat4(&keyFrames[currIdx].rotation);
    XMVECTOR currT = XMLoadFloat4(&keyFrames[currIdx].translation);

    XMVECTOR nextS = XMLoadFloat4(&keyFrames[nextIdx].scale);
    XMVECTOR nextR = XMLoadFloat4(&keyFrames[nextIdx].rotation);
    XMVECTOR nextT = XMLoadFloat4(&keyFrames[nextIdx].translation);

    outS = XMVectorLerp(currS, nextS, ratio);
    outR = QuaternionNlerp(currR, nextR, ratio);
    outT = XMVectorLerp(currT, nextT, ratio);
}

XMMATRIX Animator::GetBoneMatrix(int boneIndex)
{
    if (boneIndex < 0 || boneIndex >= mBoneCount)
        return XMMatrixIdentity();

    XMVECTOR finalS, finalR, finalT;
    XMVECTOR s1, r1, t1;
    GetInterpolatedSRT(boneIndex, mClipIndex, mFrame, mNextFrame, mFrameRatio, s1, r1, t1);

    if (mIsBlending && mPrevClipIndex >= 0)
    {
        XMVECTOR s2, r2, t2;
        GetInterpolatedSRT(boneIndex, mPrevClipIndex, mPrevFrame, mPrevNextFrame, mPrevFrameRatio, s2, r2, t2);

        finalS = XMVectorLerp(s2, s1, blendRatio);
        finalR = QuaternionNlerp(r2, r1, blendRatio);
        finalT = XMVectorLerp(t2, t1, blendRatio);
    }
    else
    {
        finalS = s1;
        finalR = r1;
        finalT = t1;
    }

    XMMATRIX matBone = XMMatrixAffineTransformation(finalS, XMVectorZero(), finalR, finalT);

    return XMMatrixMultiply(mBones[boneIndex].matOffset, matBone);
}

XMFLOAT3 Animator::GetBonePosition(int boneIndex)
{
    XMMATRIX mat = GetBoneMatrix(boneIndex);
    XMFLOAT3 pos;
    XMStoreFloat3(&pos, mat.r[3]);

    return pos;
}

XMVECTOR Animator::GetBoneRotation(int boneIndex)
{
    if (boneIndex < 0 || boneIndex >= mBoneCount)
        return XMQuaternionIdentity();

    XMVECTOR s1, r1, t1;
    GetInterpolatedSRT(boneIndex, mClipIndex, mFrame, mNextFrame,
        mFrameRatio, s1, r1, t1);

    if (mIsBlending && mPrevClipIndex >= 0)
    {
        XMVECTOR s2, r2, t2;
        GetInterpolatedSRT(boneIndex, mPrevClipIndex, mPrevFrame,
            mPrevNextFrame, mPrevFrameRatio, s2, r2, t2);
        return QuaternionNlerp(r2, r1, blendRatio);
    }

    return r1;
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

    mBoneFrameBuffer = make_unique<UploadBuffer>();
    mOffsetBuffer = make_unique<UploadBuffer>();
    mFinalBuffer = make_unique<UAVBuffer>();
    mAnimationCB = make_unique<UploadBuffer>();

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

    auto initBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        mFinalBuffer->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    core.GetGraphicsCmdList()->ResourceBarrier(1, &initBarrier);

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

    UpdateAnimationOffsets();

    mFrame = 1;
    mNextFrame = 2;
    mFrameRatio = 0.0f;

    if (mPrevClipIndex >= 0 && mPrevClipIndex < mAnimations.size()) {
        const auto& prevClip = mAnimations[mPrevClipIndex];
        const float prevFramerate = static_cast<float>(prevClip.frameCount) / prevClip.duration;
        float prevFrameFloat = (mPrevUpdateTime * prevFramerate) + 1.0f;

        mPrevFrame = static_cast<int32_t>(prevFrameFloat);
        mPrevFrame = max(1, min(mPrevFrame, prevClip.frameCount));

        if (mPrevFrame == prevClip.frameCount) {
            mPrevNextFrame = 1;
        }
        else {
            mPrevNextFrame = mPrevFrame + 1;
        }

        mPrevFrameRatio = prevFrameFloat - mPrevFrame;
    }
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

    auto toUav = CD3DX12_RESOURCE_BARRIER::Transition(
        mFinalBuffer->GetResource(),
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->ResourceBarrier(1, &toUav);

    cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::Compute));
    cmdList->SetComputeRootSignature(core.GetRootSig()->Get());
    cmdList->SetComputeRootConstantBufferView(2, mAnimationCB->GetGPUVirtualAddress());

    cmdList->SetComputeRootShaderResourceView(8, GetBoneFrameBuffer()->GetGPUVirtualAddress());
    cmdList->SetComputeRootShaderResourceView(9, GetOffsetBuffer()->GetGPUVirtualAddress());

    cmdList->SetComputeRootUnorderedAccessView(11, GetFinalBuffer()->GetGPUVirtualAddress());

    UINT groupCount = (animData.boneCount + 255) / 256;
    cmdList->Dispatch(groupCount, 1, 1);

    auto toSrv = CD3DX12_RESOURCE_BARRIER::Transition(
        mFinalBuffer->GetResource(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &toSrv);
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

int Animator::GetFrameCount() const
{
    if (mAnimations.empty() || mClipIndex < 0 || mClipIndex >= mAnimations.size())
        return 0;
    return mAnimations[mClipIndex].frameCount;
}