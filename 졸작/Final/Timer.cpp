#include "pch.h"
#include "Timer.h"

Timer& Timer::Get()
{
	static Timer timer;
	return timer;
}

void Timer::Initialize()
{
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&_frequency));
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&_prevCount));		
}

void Timer::Update()
{
	UINT64 currentCount;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currentCount));

	_deltaTime = (currentCount - _prevCount) / static_cast<float>(_frequency);
	
	constexpr float targetFrameTime = 1.0f / 240.0f;
	constexpr float epsilon = 0.0001f;

	while (_deltaTime < targetFrameTime - epsilon)
	{
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currentCount));
		_deltaTime = (currentCount - _prevCount) / static_cast<float>(_frequency);
	}

	_prevCount = currentCount;

	_frameCount++;
	_frameTime += _deltaTime;

	if (_frameTime > 1.f)
	{
		_fps = static_cast<UINT32>(_frameCount / _frameTime);

		_frameTime = 0.f;
		_frameCount = 0;
	}
}

void Timer::Reset()
{
	_frameCount = 0;
	_frameTime = 0.f;
	_fps = 0;

	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&_prevCount));
	_deltaTime = 0.f;
}
