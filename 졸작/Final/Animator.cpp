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

void Animator::VerifyAgainstBakedFile(DX12Core& core, const wstring& bakedFilePath)
{
    if (!mReadBackBuffer) {
        mReadBackBuffer = make_unique<ReadBackBuffer>();
        mReadBackBuffer->Initialize(core.GetDevice(), mBoneCount * sizeof(XMMATRIX));
    }

    // ===== 1. Compute Shader 실행 =====
    OutputDebugStringA("=== Compute Shader 실행 ===\n");

    // 특정 프레임으로 설정 (예: 프레임 30)
    int testFrame = 30;
    mUpdateTime = (testFrame - 1) * (mAnimations[mClipIndex].duration / mAnimations[mClipIndex].frameCount);
    mFrame = testFrame;
    mNextFrame = testFrame; // 정확한 프레임 (보간 없음)
    mFrameRatio = 0.0f;

    // Compute Shader 실행
    ExecuteComputeShader(core);

    // GPU 작업 완료 대기
    core.FlushCommandQueue();

    // GPU에서 CPU로 데이터 복사
    auto cmdList = core.GetGraphicsCmdList();
    mReadBackBuffer->CopyFromGPU(cmdList, mFinalBuffer->GetResource());

    core.FlushCommandQueue();
    core.ResetCommandQueue();

    // aFinal 버퍼 읽기
    vector<XMMATRIX> computedMatrices(mBoneCount);
    mReadBackBuffer->ReadData(computedMatrices.data(), mBoneCount * sizeof(XMMATRIX));

    OutputDebugStringA(("프레임 " + to_string(testFrame) + " Compute Shader 실행 완료\n").c_str());

    // ===== 2. 베이킹 파일 읽기 =====
    OutputDebugStringA("=== 베이킹 파일 읽기 ===\n");

    wifstream ifs(bakedFilePath);
    if (!ifs) {
        OutputDebugStringA("베이킹 파일 열기 실패!\n");
        return;
    }

    // 헤더 스킵
    wstring line;
    while (getline(ifs, line)) {
        if (line == L"---") break;
    }

    // 해당 프레임까지 스킵
    int targetFrameIndex = testFrame - 1; // 0-based
    for (int f = 0; f < targetFrameIndex; ++f) {
        getline(ifs, line); // "Frame[f]"
        for (int b = 0; b < mBoneCount; ++b) {
            getline(ifs, line); // "  Bone[b]:"
            for (int row = 0; row < 4; ++row) {
                getline(ifs, line); // 행렬 행
            }
        }
    }

    // 타겟 프레임 읽기
    getline(ifs, line); // "Frame[30]"

    vector<XMMATRIX> bakedMatrices(mBoneCount);
    for (int b = 0; b < mBoneCount; ++b) {
        getline(ifs, line); // "  Bone[b]:"

        for (int row = 0; row < 4; ++row) {
            getline(ifs, line);
            float m0, m1, m2, m3;
            swscanf_s(line.c_str(), L"    %f %f %f %f", &m0, &m1, &m2, &m3);
            bakedMatrices[b].r[row] = XMVectorSet(m0, m1, m2, m3);
        }
    }

    OutputDebugStringA(("프레임 " + to_string(testFrame) + " 베이킹 데이터 읽기 완료\n").c_str());

    // ===== 3. 비교 =====
    OutputDebugStringA("\n=== 비교 결과 ===\n");

    int errorCount = 0;
    float maxError = 0.0f;
    int maxErrorBone = -1;

    for (int b = 0; b < mBoneCount; ++b) {
        float boneMaxError = 0.0f;

        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                float computed = XMVectorGetByIndex(computedMatrices[b].r[row], col);
                float baked = XMVectorGetByIndex(bakedMatrices[b].r[row], col);
                float error = abs(computed - baked);

                if (error > boneMaxError) {
                    boneMaxError = error;
                }
            }
        }

        if (boneMaxError > 0.001f) {
            errorCount++;
            OutputDebugStringA(("Bone " + to_string(b) + " 오차: " + to_string(boneMaxError) + "\n").c_str());
        }

        if (boneMaxError > maxError) {
            maxError = boneMaxError;
            maxErrorBone = b;
        }
    }

    // ===== 4. 상세 출력 (가장 오차 큰 본) =====
    if (maxErrorBone >= 0) {
        OutputDebugStringA(("\n가장 큰 오차 본: " + to_string(maxErrorBone) + "\n").c_str());
        OutputDebugStringA(("최대 오차: " + to_string(maxError) + "\n\n").c_str());

        OutputDebugStringA("[Compute Shader 결과]\n");
        for (int row = 0; row < 4; ++row) {
            char buf[256];
            sprintf_s(buf, "  %.6f %.6f %.6f %.6f\n",
                XMVectorGetByIndex(computedMatrices[maxErrorBone].r[row], 0),
                XMVectorGetByIndex(computedMatrices[maxErrorBone].r[row], 1),
                XMVectorGetByIndex(computedMatrices[maxErrorBone].r[row], 2),
                XMVectorGetByIndex(computedMatrices[maxErrorBone].r[row], 3));
            OutputDebugStringA(buf);
        }

        OutputDebugStringA("\n[베이킹 파일 결과]\n");
        for (int row = 0; row < 4; ++row) {
            char buf[256];
            sprintf_s(buf, "  %.6f %.6f %.6f %.6f\n",
                XMVectorGetByIndex(bakedMatrices[maxErrorBone].r[row], 0),
                XMVectorGetByIndex(bakedMatrices[maxErrorBone].r[row], 1),
                XMVectorGetByIndex(bakedMatrices[maxErrorBone].r[row], 2),
                XMVectorGetByIndex(bakedMatrices[maxErrorBone].r[row], 3));
            OutputDebugStringA(buf);
        }
    }

    // ===== 5. 최종 판정 =====
    OutputDebugStringA("\n=== 최종 판정 ===\n");
    OutputDebugStringA(("총 본 개수: " + to_string(mBoneCount) + "\n").c_str());
    OutputDebugStringA(("오차 발견 본: " + to_string(errorCount) + "\n").c_str());
    OutputDebugStringA(("최대 오차: " + to_string(maxError) + "\n").c_str());

    if (maxError < 0.0001f) {
        OutputDebugStringA("? 검증 성공: Compute Shader와 베이킹 데이터 일치!\n");
    }
    else if (maxError < 0.001f) {
        OutputDebugStringA("?? 경고: 약간의 오차 존재 (부동소수점 오차 범위)\n");
    }
    else {
        OutputDebugStringA("? 검증 실패: 큰 오차 발견!\n");
    }
}

void Animator::ExportBakedAnimation(DX12Core& core, const wstring& outputPath, int animIndex)
{
    if (animIndex < 0 || animIndex >= mAnimations.size()) {
        OutputDebugStringA("잘못된 애니메이션 인덱스!\n");
        return;
    }

    if (!mReadBackBuffer) {
        mReadBackBuffer = make_unique<ReadBackBuffer>();
        mReadBackBuffer->Initialize(core.GetDevice(), mBoneCount * sizeof(XMMATRIX));
    }

    // 애니메이션 설정
    PlayAnimation(animIndex);
    const auto& animClip = mAnimations[animIndex];

    wofstream ofs(outputPath);
    if (!ofs) {
        OutputDebugStringA("파일 생성 실패!\n");
        return;
    }

    // 헤더 작성
    ofs << L"BAKED_ANIMATION" << endl;
    ofs << L"AnimationIndex: " << animIndex << endl;
    ofs << L"BoneCount: " << mBoneCount << endl;
    ofs << L"FrameCount: " << animClip.frameCount << endl;
    ofs << L"Duration: " << animClip.duration << endl;
    ofs << L"FPS: " << (animClip.frameCount / animClip.duration) << endl;
    ofs << L"---" << endl;

    OutputDebugStringA(("애니메이션 베이킹 시작: " + to_string(animClip.frameCount) + " 프레임\n").c_str());

    // 각 프레임마다 Compute Shader 실행 & 저장
    for (int frame = 1; frame <= animClip.frameCount; ++frame) {
        // 프레임 설정
        mFrame = frame;
        mNextFrame = frame;
        mFrameRatio = 0.0f;

        // Compute Shader 실행
        ExecuteComputeShader(core);
        core.FlushCommandQueue();

        // GPU → CPU 복사
        auto cmdList = core.GetGraphicsCmdList();
        mReadBackBuffer->CopyFromGPU(cmdList, mFinalBuffer->GetResource());
        core.FlushCommandQueue();
        core.ResetCommandQueue();

        // 데이터 읽기
        vector<XMMATRIX> finalMatrices(mBoneCount);
        mReadBackBuffer->ReadData(finalMatrices.data(), mBoneCount * sizeof(XMMATRIX));

        // 파일에 저장
        ofs << L"Frame[" << (frame - 1) << L"]" << endl;  // 0-based

        for (int b = 0; b < mBoneCount; ++b) {
            ofs << L"  Bone[" << b << L"]:" << endl;
            for (int row = 0; row < 4; ++row) {
                ofs << L"    ";
                for (int col = 0; col < 4; ++col) {
                    ofs << XMVectorGetByIndex(finalMatrices[b].r[row], col);
                    if (col < 3) ofs << L" ";
                }
                ofs << endl;
            }
        }

        // 진행상황 출력
        if (frame % 10 == 0 || frame == animClip.frameCount) {
            OutputDebugStringA(("프레임 " + to_string(frame) + "/" +
                to_string(animClip.frameCount) + " 완료\n").c_str());
        }
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
    OutputDebugStringA(("Bone Count: " + to_string(GetBoneCount()) + "\n").c_str());
    OutputDebugStringA(("Current Frame: " + to_string(GetCurrentFrame()) + "\n").c_str());
    OutputDebugStringA(("Frame Ratio: " + to_string(GetFrameRatio()) + "\n").c_str());
}
