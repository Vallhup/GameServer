#include "pch.h"
#include "Timer.h"

void Timer::Initialize()
{
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&frequency));
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&prevCount));		
}

void Timer::Update()
{
	UINT64 currentCount;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currentCount));

	deltaTime = (currentCount - prevCount) / static_cast<float>(frequency);

	prevCount = currentCount;

	frameCount++;
	frameTime += deltaTime;
	totalTime += deltaTime;

	if (frameTime > 1.f)
	{
		fps = static_cast<UINT32>(frameCount / frameTime);

		frameTime = 0.f;
		frameCount = 0;
	}
}

void Timer::Reset()
{
	frameCount = 0;
	frameTime = 0.f;
	fps = 0;

	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&prevCount));
	deltaTime = 0.f;
}
