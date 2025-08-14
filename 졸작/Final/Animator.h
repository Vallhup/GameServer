#pragma once
#include "Component.h"
#include "Importer.h"

class UploadBuffer;

class Animator : public Component
{
public:
    void Update(float deltaTime) override;

    void SetAnimationData(const vector<AnimClipInfo>& animations);  
    void SetSkeletonData(const SkeletonData& skeleton);
    void PlayAnimation(int animIndex);

    void ExecuteComputeShader();
    void LoadAnimationFromImporter(const Importer& importer);

    // Compute Shader용 버퍼들
    UploadBuffer* GetBoneFrameBuffer() const { return _boneFrameBuffer.get(); }
    UploadBuffer* GetOffsetBuffer() const { return _offsetBuffer.get(); }
    UploadBuffer* GetFinalBuffer() const { return _finalBuffer.get(); }

    int GetBoneCount() const { return _boneCount; }
    int GetCurrentFrame() const { return _frame; }
    int GetNextFrame() const { return _nextFrame; }
    float GetFrameRatio() const { return _frameRatio; }
    int GetCurrentAnimOffset() const { return _currentAnimationOffset; }

private:
    void CreateBuffers();

    vector<AnimClipInfo> _animations;  
    vector<BoneInfo> _bones;

    unique_ptr<UploadBuffer> _boneFrameBuffer;    // 키프레임 데이터
    unique_ptr<UploadBuffer> _offsetBuffer;       // 오프셋 행렬
    unique_ptr<UploadBuffer> _finalBuffer;        // 최종 본 행렬 (Compute 출력)

    int _boneCount = 0;
    int _clipIndex = 0;
    float _updateTime = 0.0f;
    int _frame = 0;
    int _nextFrame = 0;
    float _frameRatio = 0.0f;
    int _currentAnimationOffset = 0;
    bool _isInitialized = false;
};