#pragma once

#include <deque>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

using TaskFn = void(*)(void*);

class TaskGroup;

struct TaskDesc
{
	TaskFn fn{ nullptr };
	void* ctx{ nullptr };
	TaskGroup* group{ nullptr };
};

class ThreadPool final {
public:
	explicit ThreadPool(uint32_t threadCnt = 0);
	~ThreadPool();

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

	bool Submit(TaskFn fn, void* ctx);
	bool Submit(TaskFn fn, void* ctx, TaskGroup& group);
	bool Submit(TaskDesc task);

	void Wait(TaskGroup& group);

	uint32_t WorkerCount() const noexcept 
	{ 
		return static_cast<uint32_t>(_workers.size()); 
	}

private:
	void Start(uint32_t threadCnt = 1);
	void Stop();
	void WorkerLoop();

private:
	std::vector<std::thread> _workers;

	std::mutex _queueMtx;
	std::condition_variable _queueCv;
	std::deque<TaskDesc> _queue;

	std::atomic<bool> _running{ false };
	std::atomic<bool> _stopping{ false };
};

// 1. Work-Stealing
// 2. Lock-Free Deque
// 3. Fiber
// 4. Nested Continuation

// 1. FrameJobGraph 자료구조 정의
// 2. ThreadPool -> FrameJobExecutor 확장
// 3. SystemMeta 기반 dependency builder 구현
// 4. ExecutionGraphScheduler(world-local fragment builder) 구현
// 5. WorldScheduler에서 selected worlds + dt policy 산출
// 6. WorldRuntime 최소 표면 정리
// 7. WorldInstance가 Runtime + ExecutionModel 소유하도록 정리
// 8. PostFrame lifecycle flush / transfer-admission reconcile 연결