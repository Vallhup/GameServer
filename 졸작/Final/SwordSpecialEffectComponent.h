#pragma once
#include "Component.h"

class Animator;
class AnimationMachine;
class Transform;

// 캐릭터의 검 본을 매 프레임 추적해 스워드 이펙트 위치/회전을 갱신하고,
// 패링 성공 후 스페셜 공격(AttackSpecial 클립) 진입 순간에 이펙트를 재생한다.
// 쌍검 캐릭터를 위해 본 인덱스를 여러 개(검마다 1개) 받을 수 있다.
class SwordSpecialEffectComponent : public Component
{
public:
	void Init() override;
	void Update(float deltaTime) override;

	void SetEffectName(const wstring& name) { effectName = name; }

	// 검 본 인덱스(스켈레톤마다 다름). 쌍검이면 두 검의 본을 모두 전달
	void SetBoneIndices(const vector<int>& indices);

private:
	Animator* animator = nullptr;
	AnimationMachine* animMachine = nullptr;
	Transform* transform = nullptr;

	wstring effectName;
	vector<int> boneIndices{ 45 };
	vector<int> handles;            // boneIndices와 1:1 대응, 미재생 시 -1
	bool wasPlayingSpecial = false;
};
