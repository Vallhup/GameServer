#pragma once

#include <deque>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

using TaskFn = void(*)(void*);

struct TaskCounter
{
	std::atomic<int> remaining{ 0 };
	std::mutex mtx;
	std::condition_variable cv;

	std::exception_ptr firstException{ nullptr };

	inline void RecordException(std::exception_ptr eptr)
	{
		std::lock_guard lock{ mtx };
		
		if (!firstException)
			firstException = eptr;
	}

	inline void FinishTask()
	{
		if (remaining.fetch_sub(1, std::memory_order_acq_rel) == 1)
		{
			std::lock_guard lock{ mtx };
			cv.notify_all();
		}
	}
};

struct TaskDesc
{
	TaskFn fn{ nullptr };
	void* ctx{ nullptr };
	TaskCounter* counter{ nullptr };
};

class ThreadPool final {
public:
	explicit ThreadPool(uint32_t threadCnt = 1);
	~ThreadPool();

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

	bool Submit(TaskFn fn, void* ctx);
	bool Submit(TaskFn fn, void* ctx, TaskCounter& counter);
	bool Submit(TaskDesc task);

	void Wait(TaskCounter& counter);

	uint32_t WorkerCount() const noexcept { return static_cast<uint32_t>(_workers.size()); }

private:
	void Start(uint32_t threadCnt = 1);
	void Stop();

	void WorkerLoop();

	std::vector<std::thread> _workers;

	std::mutex _queueMtx;
	std::condition_variable _queueCv;
	std::deque<TaskDesc> _queue;

	std::atomic<bool> _running;
	std::atomic<bool> _stopping;
};

// 1. Work-Stealing
// 2. Lock-Free Deque
// 3. Fiber
// 4. Nested Continuation