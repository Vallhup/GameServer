#include "pch.h"
#include "TimerManager.h"

TimerManager::TimerManager(int intervalMs) : _intervalMs(intervalMs), _running(false)
{
}

TimerManager::~TimerManager()
{
	Stop();
}

void TimerManager::Register(const std::function<void(float)>& tickFunc)
{
	_tickFuncs.push_back(tickFunc);
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

		for (auto& func : _tickFuncs) {
			func(delta);
		}

		std::this_thread::sleep_for(milliseconds(_intervalMs));
	}
}