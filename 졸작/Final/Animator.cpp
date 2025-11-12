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
    mFinalBuffer = make_unique<UAVBuffer>();
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

    D3D12_HEAP_PROPERTIES readbackHeap = {};
    readbackHeap.Type = D3D12_HEAP_TYPE_READBACK;
    readbackHeap.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    readbackHeap.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = mBoneCount * sizeof(XMMATRIX);
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    core.GetDevice()->CreateCommittedResource(
        &readbackHeap,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&mDebugReadbackBuffer)
    );

    // Fence 생성
    core.GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mDebugFence));
    mDebugFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

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

    cmdList->SetComputeRootShaderResourceView(8, GetBoneFrameBuffer()->GetGPUVirtualAddress());  // 레지 넘버링 부분
    cmdList->SetComputeRootShaderResourceView(9, GetOffsetBuffer()->GetGPUVirtualAddress());     // 레지 넘버링 부분

    cmdList->SetComputeRootUnorderedAccessView(11, GetFinalBuffer()->GetGPUVirtualAddress());     // 레지 넘버링 부분

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
    OutputDebugStringA(("Bone Count: " + to_string(mBoneCount) + "\n").c_str());
    OutputDebugStringA(("Current Frame: " + to_string(mFrame) + "\n").c_str());
    OutputDebugStringA(("Frame Ratio: " + to_string(mFrameRatio) + "\n").c_str());
}

void Animator::DebugPrintBoneMatrix(DX12Core& core, int boneIndex)
{
    auto cmdList = core.GetGraphicsCmdList();

    // 1. UAV → Copy Source
    D3D12_RESOURCE_BARRIER barriers[2] = {};
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.pResource = mFinalBuffer->GetResource();
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    cmdList->ResourceBarrier(1, &barriers[0]);

    // 2. GPU → Readback 복사
    cmdList->CopyResource(mDebugReadbackBuffer.Get(), mFinalBuffer->GetResource());

    // 3. Copy Source → UAV
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    cmdList->ResourceBarrier(1, &barriers[0]);

    // 4. Command List 실행
    cmdList->Close();
    ID3D12CommandList* cmdLists[] = { cmdList };
    core.GetCmdQueue()->ExecuteCommandLists(1, cmdLists);

    // 5. Fence 대기
    const UINT64 fenceValue = ++mDebugFenceValue;
    core.GetCmdQueue()->Signal(mDebugFence.Get(), fenceValue);
    if (mDebugFence->GetCompletedValue() < fenceValue) {
        mDebugFence->SetEventOnCompletion(fenceValue, mDebugFenceEvent);
        WaitForSingleObject(mDebugFenceEvent, INFINITE);
    }

    // 6. CPU에서 읽기
    XMMATRIX* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, mBoneCount * sizeof(XMMATRIX) };
    mDebugReadbackBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedData));

    // 7. 출력
    static int frameCount = 0;
    char debugMsg[1024];

    const auto& currentAnim = mAnimations[mClipIndex];
    sprintf_s(debugMsg, "=== Frame %d, Bone %d ===\n", frameCount++, boneIndex);
    OutputDebugStringA(debugMsg);

    sprintf_s(debugMsg, "Animation: %s\n", currentAnim.animName.c_str());
    OutputDebugStringA(debugMsg);

    sprintf_s(debugMsg, "AnimFrame: %d/%d (Ratio: %.3f)\n",
        mFrame, currentAnim.frameCount, mFrameRatio);
    OutputDebugStringA(debugMsg);

    sprintf_s(debugMsg, "Time: %.3f/%.3f sec\n", mUpdateTime, currentAnim.duration);
    OutputDebugStringA(debugMsg);

    OutputDebugStringA("Matrix:\n");

    XMMATRIX& m = mappedData[boneIndex];
    for (int row = 0; row < 4; row++) {
        sprintf_s(debugMsg, "[%.6f, %.6f, %.6f, %.6f]\n",
            XMVectorGetX(m.r[row]), XMVectorGetY(m.r[row]),
            XMVectorGetZ(m.r[row]), XMVectorGetW(m.r[row]));
        OutputDebugStringA(debugMsg);
    }
    OutputDebugStringA("\n");

    D3D12_RANGE writeRange = { 0, 0 };
    mDebugReadbackBuffer->Unmap(0, &writeRange);

    // 8. Command List 리셋
    core.ResetCommandQueue();
}