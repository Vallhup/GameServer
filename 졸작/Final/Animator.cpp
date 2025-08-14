#include "pch.h"
#include "Animator.h"
#include "UploadBuffer.h"
#include "DX12Graphics.h"
#include "Device.h"

void Animator::Update(float deltaTime)
{
    if (_animations.empty()) return;

    _currentAnimationOffset = 0;
    for (int i = 0; i < _clipIndex; ++i) {
        _currentAnimationOffset += _animations[i].keyFrames.size();
    }

    _updateTime += deltaTime;
    const auto& animClip = _animations[_clipIndex];

    if (_updateTime >= animClip.duration) {
        _updateTime = 0.0f;
    }

    // 레퍼런스와 동일한 프레임 계산
    const int32_t ratio = static_cast<int32_t>(animClip.frameCount / animClip.duration);
    _frame = static_cast<int32_t>(_updateTime * ratio);
    _frame = min(_frame, animClip.frameCount - 1);
    _nextFrame = min(_frame + 1, animClip.frameCount - 1);
    _frameRatio = static_cast<float>(_updateTime * ratio - _frame);  // 소수점 부분
}

void Animator::SetAnimationData(const vector<AnimClipInfo>& animations)  // 변경
{
    _animations = animations;
    if (!_animations.empty()) {
        _boneCount = static_cast<int>(_animations[0].keyFrames.size() / _animations[0].frameCount);
        CreateBuffers();
    }
}

void Animator::SetSkeletonData(const SkeletonData& skeleton)
{
    _bones = skeleton.bones;

    if (_offsetBuffer && !_bones.empty()) {
        // ★ 실제 오프셋 행렬 사용
        vector<XMMATRIX> offsetMatrices;
        for (const auto& bone : _bones) {
            offsetMatrices.push_back(bone.matOffset);  // 실제 데이터 사용
        }
        _offsetBuffer->CopyData(offsetMatrices.data(), offsetMatrices.size() * sizeof(XMMATRIX));
    }
}

void Animator::CreateBuffers()
{
    if (_animations.empty()) return;

    // 버퍼 생성
    _boneFrameBuffer = make_unique<UploadBuffer>();
    _offsetBuffer = make_unique<UploadBuffer>();
    _finalBuffer = make_unique<UploadBuffer>();

    // BoneFrame 버퍼 - 레퍼런스와 동일한 구조
    size_t totalKeyFrames = 0;
    for (const auto& anim : _animations) {
        totalKeyFrames += anim.keyFrames.size();
    }

    _boneFrameBuffer->Initialize(
        GET(DX12Graphics).GetDevice()->GetDevice().Get(),
        totalKeyFrames * sizeof(AnimFrameParams)
    );

    _offsetBuffer->Initialize(
        GET(DX12Graphics).GetDevice()->GetDevice().Get(),
        _boneCount * sizeof(XMMATRIX)
    );

    _finalBuffer->Initialize(
        GET(DX12Graphics).GetDevice()->GetDevice().Get(),
        _boneCount * sizeof(XMMATRIX)
    );

    // 모든 애니메이션 데이터를 하나의 버퍼에 복사
    vector<AnimFrameParams> allFrameData;
    for (const auto& anim : _animations) {
        allFrameData.insert(allFrameData.end(), anim.keyFrames.begin(), anim.keyFrames.end());
    }

    _boneFrameBuffer->CopyData(allFrameData.data(), allFrameData.size() * sizeof(AnimFrameParams));
    _isInitialized = true;
}

void Animator::PlayAnimation(int animIndex)
{
    if (animIndex >= 0 && animIndex < _animations.size()) {
        _clipIndex = animIndex;
        _updateTime = 0.0f;
    }
}