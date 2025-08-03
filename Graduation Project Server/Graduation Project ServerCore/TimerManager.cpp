#include "pch.h"
#include "TimerManager.h"

TimerManager::TimerManager() : _running(false)
{
}

TimerManager::~TimerManager()
{
	Stop();
}

bool TimerManager::Start()
{
	bool expected{ false };
	if (_running.compare_exchange_strong(expected, true)) {
		_repeatedTaskThread = std::thread([this]() { this->RepeatedTaskThreadLoop(); });
		_oneTimeTaskThread = std::thread([this]() { this->OneTimeTaskThreadLoop(); });

		return true;
	}

	return false;
}

void TimerManager::Stop()
{
	bool expected{ true };
	if (_running.compare_exchange_strong(expected, false)) {
		if (_repeatedTaskThread.joinable()) {
			_repeatedTaskThread.join();
		}

		if (_oneTimeTaskThread.joinable()) {
			_oneTimeTaskThread.join();
		}
	}
}

void TimerManager::AddRepeatedTask(const std::function<void(float)>& func, float intervalMs)
{
	_repeatedTasks.emplace_back(func, std::chrono::milliseconds(static_cast<int>(intervalMs)));
}

void TimerManager::AddOneTimeTask(const std::function<void()>& func, float delayMs)
{
	using namespace std::chrono;

	OneTimeTask task{ func, high_resolution_clock::now() + milliseconds(static_cast<int>(delayMs)) };
	_oneTimeTaskQueue.push(task);
}

void TimerManager::RepeatedTaskThreadLoop()
{
	using namespace std::chrono;

	while (_running) {
		auto now = high_resolution_clock::now();

		for (auto& task : _repeatedTasks) {
			while (now >= task.nextExecTime) {
				float deltaTime = duration<float>(task.interval).count();
				task.func(deltaTime);
				task.nextExecTime += task.interval;
			}
		}
	}
}

void TimerManager::OneTimeTaskThreadLoop()
{
	using namespace std::chrono;

	while (_running) {
		OneTimeTask task;
		while (_oneTimeTaskQueue.try_pop(task)) {
			if (task.targetTime > high_resolution_clock::now()) {
				_oneTimeTaskQueue.push(task);
				break;
			}

			task.func();
		}
	}
}