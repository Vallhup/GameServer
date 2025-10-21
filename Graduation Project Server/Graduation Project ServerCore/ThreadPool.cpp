#include "pch.h"
#include "ThreadPool.h"

void ThreadPool::Start(size_t threadCount)
{
	bool expected{ false };
	if (_running.compare_exchange_strong(expected, true)) {
		for (size_t i = 0; i < threadCount; ++i) {
			_workers.emplace_back([this]() { WorkerLoop(); });
		}
	}
}

void ThreadPool::Start(size_t threadCount, std::function<void()> func)
{
	bool expected{ false };
	if (_running.compare_exchange_strong(expected, true)) {
		_workers.reserve(threadCount);
		for (size_t i = 0; i < threadCount; ++i) {
			_workers.emplace_back(func);
		}
	}
}

void ThreadPool::Stop()
{
	bool expected{ true };
	if (_running.compare_exchange_strong(expected, false)) {
		for (auto& worker : _workers) {
			if (worker.joinable()) {
				worker.join();
			}
		}
	}
}

void ThreadPool::Enqueue(std::function<void()> task)
{
	{
		std::unique_lock lock(_mutex);
		_tasks.push(std::move(task));
	}
	_cv.notify_one();
}

void ThreadPool::WorkerLoop()
{
	while (_running) {
		std::function<void()> task;
		{
			std::unique_lock lock(_mutex);
			_cv.wait(lock, [this]() { return !_tasks.empty() || !_running; });
			if (!_running && _tasks.empty()) return;
			task = std::move(_tasks.front());
			_tasks.pop();
		}
		task();
	}
}
