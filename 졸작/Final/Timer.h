#pragma once


class Timer
{
public:
	static Timer& Get();

	void Initialize();
	void Update();
	void Reset();

	UINT32 GetFps() { return fps; }
	float GetDeltaTime() { return deltaTime; }

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
