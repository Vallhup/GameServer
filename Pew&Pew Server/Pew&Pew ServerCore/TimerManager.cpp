#include "pch.h"
#include "TimerManager.h"

TimerManager::TimerManager() : _running(false)
{
}

TimerManager::~TimerManager()
{
	Stop();
}

void TimerManager::Register(const std::function<void(float)>& func, float intervalMs)
{
	_tasks.push_back(TimerTask{ func, intervalMs, 0 });
}

void TimerManager::RegisterOnce(const std::function<void()>& func, float delayMs)
{
	_tasks.push_back(TimerTask{ [func, called = false](float) mutable
		{
			if (not called) {
				func();
				called = true;
			}
		}, delayMs, 0 });
}

void TimerManager::Start()
{
	_running.store(true);
	_thread = std::thread([this]() { this->Run(); });
}

void TimerManager::Stop()
{
	_running.store(false);
	if (_thread.joinable()) {
		_thread.join();
	}
}

void TimerManager::Run()
{
	using namespace std::chrono;

	auto prev = high_resolution_clock::now();
	while (_running) {
		auto now = high_resolution_clock::now();
		float delta = duration<float>(now - prev).count();
		prev = now;

		for (auto& task : _tasks) {
			task.elapsed += delta * 1000.0f;
			if (task.elapsed >= task.intervalMs) {
				task.func(task.elapsed / 1000.0f);
				task.elapsed = 0;
			}
		}
	}
}

float GetNowTime()
{
	using namespace std::chrono;
	static const auto start = high_resolution_clock::now();
	auto now = high_resolution_clock::now();
	return duration<float>(now - start).count();
}