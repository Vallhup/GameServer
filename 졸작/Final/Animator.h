#pragma once
#include "Component.h"
#include "Importer.h"

class DX12Core;
class UploadBuffer;

class Animator : public Component
{
public:
    void Update(float deltaTime) override;
    
    // 애니메이션 블렌딩 업데이트 함수 2개
    void UpdateCurrentAnimation(float deltaTime);
    void UpdatePrevAnimation(float deltaTime);

    void SetAnimationData(DX12Core& core, const vector<AnimClipInfo>& animations);
    void SetSkeletonData(const SkeletonData& skeleton);

    // 블렌딩 없는 애니메이션
    void PlayAnimation(int animIndex);
    // 블렌딩 있는 애니메이션
    void TransitionToAnimation(int animIndex, float Duration);

    void ExecuteComputeShader(DX12Core& core);
    void LoadAnimationFromImporter(DX12Core& core, const Importer& importer);

    void DebugAnimationInfo();

    // Compute Shader용 버퍼들
    UploadBuffer* GetBoneFrameBuffer() const { return mBoneFrameBuffer.get(); }
    UploadBuffer* GetOffsetBuffer() const { return mOffsetBuffer.get(); }
    UploadBuffer* GetFinalBuffer() const { return mFinalBuffer.get(); }

    int GetBoneCount() const { return mBoneCount; }
    int GetCurrentFrame() const { return mFrame; }
    int GetNextFrame() const { return mNextFrame; }
    float GetFrameRatio() const { return mFrameRatio; }
    int GetCurrentAnimOffset() const { return mCurrentAnimOffset; }

    // 애니메이션 블렌딩 Getter
    bool IsBlending() const { return mIsBlending; }         
    int GetPrevCurrentFrame() const { return mPrevFrame; }
    int GetPrevNextFrame() const { return mPrevNextFrame; }
    float GetPrevFrameRatio() const { return mPrevFrameRatio; }
    int GetPrevAnimOffset() const { return mPrevAnimOffset; }
    float GetBlendRatio() const { return blendRatio; }

private:
    void CreateBuffers(DX12Core& core);

    vector<AnimClipInfo> mAnimations;  
    vector<BoneInfo> mBones;

    unique_ptr<UploadBuffer> mBoneFrameBuffer;    // 키프레임 데이터
    unique_ptr<UploadBuffer> mOffsetBuffer;       // 오프셋 행렬
    unique_ptr<UploadBuffer> mFinalBuffer;        // 최종 본 행렬 (Compute 출력)

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
};