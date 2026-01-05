#pragma once
#include "Singleton.h"

class Timer : public Singleton<Timer>
{
	friend class Singleton<Timer>;
	Timer() = default;

public:
	void Initialize();
	void Update();
	void Reset();

	UINT32 GetFps() const { return fps; }
	float GetDeltaTime() const { return deltaTime; }

	void SetTargetFPS(float fps);

private:
	UINT64	frequency = 0;
	UINT64	prevCount = 0;
	float	deltaTime = 0.f;

private:
	UINT32	frameCount = 0;
	float	frameTime = 0.f;
	UINT32	fps = 0;

	float targetFPS = 60.0f;
};
