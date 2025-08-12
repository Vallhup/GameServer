#include "pch.h"
#include "Animator.h"
#include "UploadBuffer.h"
#include "DX12Graphics.h"
#include "Device.h"

void Animator::Update(float deltaTime)
{
    if (animations.empty()) return;

    animationTime += deltaTime;
    const auto& currentAnim = animations[currentAnimIndex];

    if (animationTime >= currentAnim.duration) {
        animationTime = 0.0f;
    }

    int totalFrames = static_cast<int>(currentAnim.boneKeyFrames[0].size());

    float frameFloat = (animationTime / currentAnim.duration) * totalFrames;
    currentFrame = static_cast<int>(frameFloat);
    nextFrame = (currentFrame + 1) % totalFrames;  
    frameRatio = frameFloat - currentFrame;  
}

void Animator::InitializeBuffers()
{
    // BoneFrame 버퍼 크기 계산
    size_t totalKeyFrames = 0;
    for (const auto& anim : animations) {
        totalKeyFrames += anim.boneKeyFrames.size() * anim.boneKeyFrames[0].size();
    }

    // 초기화
    boneFrameBuffer->Initialize(GET(DX12Graphics).GetDevice()->GetDevice().Get(),
        totalKeyFrames * sizeof(AnimFrameParams));

    offsetBuffer->Initialize(GET(DX12Graphics).GetDevice()->GetDevice().Get(),
        boneCount * sizeof(XMMATRIX));

    finalBuffer->Initialize(GET(DX12Graphics).GetDevice()->GetDevice().Get(),
        boneCount * sizeof(XMMATRIX));

    // AnimationData → AnimFrameParams 변환
    vector<AnimFrameParams> frameData;

    for (const auto& anim : animations) {
        for (int boneIdx = 0; boneIdx < boneCount; ++boneIdx) {
            for (const auto& keyFrame : anim.boneKeyFrames[boneIdx]) {
                AnimFrameParams params;

                // XMMATRIX → SQT 분해
                XMVECTOR scale, rotation, translation;
                XMMatrixDecompose(&scale, &rotation, &translation, keyFrame.transform);

                XMStoreFloat4(&params.scale, scale);
                XMStoreFloat4(&params.rotation, rotation);
                XMStoreFloat4(&params.translation, translation);

                frameData.push_back(params);
            }
        }
    }

    // 버퍼에 복사
    boneFrameBuffer->CopyData(frameData.data(), frameData.size() * sizeof(AnimFrameParams));
}

void Animator::SetAnimationData(const vector<AnimationData>& anims)
{
    animations = anims;
    if (!animations.empty()) {
        boneCount = static_cast<int>(animations[0].boneKeyFrames.size());

        if (!isInitialized) {
            // 여기서 UploadBuffer 생성 및 초기화
            boneFrameBuffer = make_unique<UploadBuffer>();
            offsetBuffer = make_unique<UploadBuffer>();
            finalBuffer = make_unique<UploadBuffer>();

            // 실제 데이터로 초기화
            InitializeBuffers();
            isInitialized = true;
        }
    }
}

void Animator::SetSkeletonData(const SkeletonData& skeleton)
{
    if (offsetBuffer) {
        // 오프셋 행렬들을 버퍼에 복사
        vector<XMMATRIX> offsetMatrices;
        for (const auto& bone : skeleton.bones) {
            offsetMatrices.push_back(bone.offsetMatrix);
        }
        offsetBuffer->CopyData(offsetMatrices.data(), offsetMatrices.size() * sizeof(XMMATRIX));
    }
}

void Animator::PlayAnimation(int animIndex)
{
    if (animIndex >= 0 && animIndex < animations.size()) {
        currentAnimIndex = animIndex;
        animationTime = 0.0f;  
    }
}