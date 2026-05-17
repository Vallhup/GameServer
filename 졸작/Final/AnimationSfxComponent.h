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

class AnimationSfxComponent : public Component
{
public:
	void Update(float deltaTime) override;

	void AddTrigger(const string& clip, int frameLo, int frameHi, const char* sound);

private:
	vector<SfxTrigger> triggers;
};
