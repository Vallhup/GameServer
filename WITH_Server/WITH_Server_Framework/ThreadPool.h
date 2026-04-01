#pragma once

#include <deque>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

using WorkerPumpFn = bool(*)(void*);

class ThreadPool final {
public:
	ThreadPool() = default;
	~ThreadPool();

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

public:
	bool Start(uint32_t threadCnt, WorkerPumpFn pump, void* pumpCtx);
	void Stop() noexcept;

	void WakeOne() noexcept;
	void WakeAll() noexcept;

	[[nodiscard]]
	bool IsRunning() const noexcept
	{
		return _running.load();
	}

	[[nodiscard]]
	uint32_t WorkerCount() const noexcept
	{
		return static_cast<uint32_t>(_workers.size());
	}

private:
	void WorkerLoop();

private:
	std::vector<std::thread> _workers;

	std::mutex _cvMtx;
	std::condition_variable _cv;

	std::atomic<bool> _running{ false };
	std::atomic<bool> _stopping{ false };
	std::atomic<uint64_t> _workEpoch{ 0 };

	WorkerPumpFn _pump{ nullptr };
	void* _pumpCtx{ nullptr };
};

// 1. Work-Stealing
// 2. Lock-Free Deque
// 3. Fiber
// 4. Nested Continuation