#pragma once
#include "Component.h"

class Animator;
class AnimationMachine;
class Transform;

class SwordSpecialEffectComponent : public Component
{
public:
	void Init() override;
	void Update(float deltaTime) override;

	void SetEffectName(const wstring& name) { effectName = name; }

	void SetBoneIndices(const vector<int>& indices);

private:
	Animator* animator = nullptr;
	AnimationMachine* animMachine = nullptr;
	Transform* transform = nullptr;

	wstring effectName;
	vector<int> boneIndices{ 45 };
	vector<int> handles;           
	bool wasPlayingSpecial = false;
};
