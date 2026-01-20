#pragma once
#include "Component.h"
#include "Importer.h"
#include <wrl/client.h>

class DX12Core;

struct AnimationConstants
{
    int boneCount;
    int currentFrame;
    int nextFrame;
    float ratio;
    int animationOffset;

    int isBlending;
    int prevCurrentFrame;
    int prevNextFrame;
    float prevRatio;
    int prevAnimationOffset;
    float blendRatio;

    float padding;
};

class Animator : public Component
{
public:
    void Update(float deltaTime) override;

    void SetAnimationData(DX12Core& core, const vector<AnimClipInfo>& animations);
    void SetSkeletonData(const SkeletonData& skeleton);

    void PlayAnimation(int animIndex);
    void TransitionToAnimation(int animIndex, float Duration);

    void ExecuteComputeShader(DX12Core& core);
    void LoadAnimationFromImporter(DX12Core& core, const Importer& importer);

    void DebugAnimationInfo();

    // Compute Shader�� ���۵�
    UploadBuffer* GetBoneFrameBuffer() const { return mBoneFrameBuffer.get(); }
    UploadBuffer* GetOffsetBuffer() const { return mOffsetBuffer.get(); }
    UAVBuffer* GetFinalBuffer() const { return mFinalBuffer.get(); }

    float GetAnimationSpeed() const { return animationSpeed; }
    void SetAnimationSpeed(float speed) { animationSpeed = speed; }

private:
    void CreateBuffers(DX12Core& core);

    void UpdateCurrentAnimation(float deltaTime);
    void UpdatePrevAnimation(float deltaTime);

private:
    vector<AnimClipInfo> mAnimations; 
    vector<BoneInfo> mBones;

    unique_ptr<UploadBuffer> mBoneFrameBuffer;    // Ű������ ������
    unique_ptr<UploadBuffer> mOffsetBuffer;       // ������ ���
    unique_ptr<UAVBuffer> mFinalBuffer;        // ���� �� ��� (Compute ���)
    unique_ptr<UploadBuffer> mAnimationCB;

    int mBoneCount = 0;
    int mClipIndex = 0;
    float mUpdateTime = 0.0f;
    int mFrame = 0;
    int mNextFrame = 0;
    float mFrameRatio = 0.0f;
    int mCurrentAnimOffset = 0;

    int mPrevClipIndex = -1;
    float mPrevUpdateTime = 0.0f;
    int mPrevFrame = 0;
    int mPrevNextFrame = 0;
    float mPrevFrameRatio = 0.0f;
    int mPrevAnimOffset = 0;

    float blendTime = 0.0f;
    float blendDuration = 0.3f;
    float blendRatio = 0.0f;

    bool mIsInitialized = false;
    bool mIsBlending = false;

    float animationSpeed = 1.0f;
};