#pragma once
#include "Component.h"
#include "Importer.h"

class UploadBuffer;

struct AnimFrameParams
{
    XMFLOAT4 scale;
    XMFLOAT4 rotation;
    XMFLOAT4 translation;
};

class Animator : public Component
{
public:
    void Update(float deltaTime) override;

    // 애니메이션 설정
    void InitializeBuffers();
    void SetAnimationData(const vector<AnimationData>& animations);
    void SetSkeletonData(const SkeletonData& skeleton);
    void PlayAnimation(int animIndex);

    // 현재 애니메이션 정보 getter
    UploadBuffer* GetBoneFrameBuffer() const { return boneFrameBuffer.get(); }
    UploadBuffer* GetOffsetBuffer() const { return offsetBuffer.get(); }
    UploadBuffer* GetFinalBuffer() const { return finalBuffer.get(); }

    int GetBoneCount() const { return boneCount; }
    int GetCurrentFrame() const { return currentFrame; }
    int GetNextFrame() const { return nextFrame; }
    float GetFrameRatio() const { return frameRatio; }

private:
    vector<AnimationData> animations;  // Importer에서 가져온 데이터

    unique_ptr<UploadBuffer> boneFrameBuffer;  // 키프레임 데이터
    unique_ptr<UploadBuffer> offsetBuffer;     // 오프셋 행렬
    unique_ptr<UploadBuffer> finalBuffer;      // 최종 본 행렬 (Compute 결과)

    int boneCount = 0;
    int currentAnimIndex = 0;
    float animationTime = 0.0f;

    int currentFrame = 0;
    int nextFrame = 0;
    float frameRatio = 0.0f;

    bool isInitialized = false;
};