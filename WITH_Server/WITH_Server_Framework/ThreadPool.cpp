#include "pch.h"
#include "ThreadPool.h"

ThreadPool::ThreadPool(uint32_t threadCnt) : _running(false), _stopping(false)
{
	Start(threadCnt);
}

ThreadPool::~ThreadPool()
{
	Stop();
}

void ThreadPool::Start(uint32_t threadCnt) 
{
	if (_running.load(std::memory_order_acquire))
		return;

	if (threadCnt == 0)
	{
		threadCnt = std::max(1u, std::thread::hardware_concurrency());
	}

	_stopping.store(false, std::memory_order_release);
	_running.store(true, std::memory_order_release);

	_workers.reserve(threadCnt);
	for (uint32_t i = 0; i < threadCnt; ++i)
	{
		_workers.emplace_back([this]() { WorkerLoop(); });
	}
}

void ThreadPool::Stop()
{
	if (!_running.exchange(false, std::memory_order_acq_rel))
		return;

	_stopping.store(true, std::memory_order_release);
	_queueCv.notify_all();

	for (std::thread& worker : _workers)
	{
		if (worker.joinable())
			worker.join();
	}
	_workers.clear();
	_queue.clear();
}

bool ThreadPool::Submit(TaskFn fn, void* ctx)
{
	return Submit(TaskDesc{ fn, ctx, nullptr });
}

bool ThreadPool::Submit(TaskFn fn, void* ctx, TaskCounter& counter)
{
	return Submit(TaskDesc{ fn, ctx, &counter });
}

bool ThreadPool::Submit(TaskDesc task)
{
	if (!task.fn) return false;

	{
		std::lock_guard lock{ _queueMtx };

		if (!_running.load(std::memory_order_acquire) ||
			_stopping.load(std::memory_order_acquire))
			return false;

		_queue.push_back(task);

		if (task.counter)
			task.counter->remaining.fetch_add(1, std::memory_order_relaxed);
	}

	_queueCv.notify_one();
	return true;
}

void ThreadPool::Wait(TaskCounter& counter)
{
	if (counter.remaining.load(std::memory_order_acquire) != 0)
	{
		std::unique_lock lock{ counter.mtx };
		counter.cv.wait(lock,
			[&]() { return counter.remaining.load(std::memory_order_acquire) == 0; });
	}

	if (counter.firstException)
		std::rethrow_exception(counter.firstException);
}

void ThreadPool::WorkerLoop()
{
	while (true)
	{
		TaskDesc task;

		{
			std::unique_lock lock{ _queueMtx };
			_queueCv.wait(lock, 
				[&]() { return _stopping.load() || !_queue.empty(); });

			if (_stopping.load() && _queue.empty())
				return;

			task = _queue.front();
			_queue.pop_front();
		}

		try
		{
			task.fn(task.ctx);
		}

		catch (const std::exception& ex)
		{
			if (task.counter)
			{
				task.counter->RecordException(std::current_exception());
			}

			else
			{
				throw std::runtime_error("Exception occurred in Task without Counter: " + std::string(ex.what()));
			}
		}

		if (task.counter)
			task.counter->FinishTask();
	}
}