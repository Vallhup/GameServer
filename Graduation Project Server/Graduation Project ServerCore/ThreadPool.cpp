#include "pch.h"
#include "ThreadPool.h"

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
