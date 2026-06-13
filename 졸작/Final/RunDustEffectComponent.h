#pragma once
#include "Component.h"

class Animator;
class AnimationMachine;
class Transform;

class RunDustEffectComponent : public Component
{
public:
	void Init() override;
	void Update(float deltaTime) override;

	void AddFootstep(int frameLo, int frameHi, int boneIndex);

private:
	struct Footstep
	{
		int frameLo;
		int frameHi;
		int boneIndex;
		bool fired = false;
	};

	Animator* animator = nullptr;
	AnimationMachine* animMachine = nullptr;
	Transform* transform = nullptr;

	vector<Footstep> footsteps;
};
