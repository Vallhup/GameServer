#pragma once
#include "Component.h"

struct SfxTrigger
{
	string clip;
	int frameLo;
	int frameHi;
	const char* sound;
	bool fired = false;
};

struct AnimEffectTrigger
{
	string clip;
	int frameLo;
	int frameHi;
	wstring effect;
	bool fired = false;
};

class AnimationSfxComponent : public Component
{
public:
	void Update(float deltaTime) override;

	void AddTrigger(const string& clip, int frameLo, int frameHi, const char* sound);
	void AddEffectTrigger(const string& clip, int frameLo, int frameHi, const wstring& effect);

private:
	vector<SfxTrigger> triggers;
	vector<AnimEffectTrigger> effectTriggers;
};
