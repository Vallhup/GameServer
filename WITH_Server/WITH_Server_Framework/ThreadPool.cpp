#include "pch.h"
#include "ThreadPool.h"
#include "TaskGroup.h"

#include <algorithm>
#include <stdexcept>
#include <string>

ThreadPool::ThreadPool(uint32_t threadCnt)
{
	Start(threadCnt);
}

ThreadPool::~ThreadPool()
{
	Stop();
}

void ThreadPool::Start(uint32_t threadCnt) 
{
	if (_running.exchange(true))
		return;

	if (threadCnt == 0)
	{
		threadCnt = std::max(1u, std::thread::hardware_concurrency());
	}

	_stopping.store(false);

	_workers.reserve(threadCnt);
	for (uint32_t i = 0; i < threadCnt; ++i)
	{
		_workers.emplace_back([this]() { WorkerLoop(); });
	}
}

void ThreadPool::Stop()
{
	if (!_running.exchange(false))
		return;

	_stopping.store(true);
	_queueCv.notify_all();

	for (std::thread& worker : _workers)
	{
		if (worker.joinable())
			worker.join();
	}
	_workers.clear();

	{
		std::lock_guard lock{ _queueMtx };

		while (!_queue.empty())
		{
			TaskDesc& task = _queue.front();

			if (task.group)
				task.group->RollbackTaskSlot();

			_queue.pop_front();
		}
	}
}

bool ThreadPool::Submit(TaskFn fn, void* ctx)
{
	return Submit(TaskDesc{ fn, ctx, nullptr });
}

bool ThreadPool::Submit(TaskFn fn, void* ctx, TaskGroup& group)
{
	return Submit(TaskDesc{ fn, ctx, &group });
}

bool ThreadPool::Submit(TaskDesc task)
{
	if (!task.fn) 
		return false;

	if (task.group && !task.group->TryAcquireTaskSlot())
		return false;

	{
		std::lock_guard lock{ _queueMtx };

		if (!_running.load() || _stopping.load())
		{
			if (task.group)
				task.group->RollbackTaskSlot();
			return false;
		}

		_queue.push_back(task);
	}

	_queueCv.notify_one();
	return true;
}

void ThreadPool::Wait(TaskGroup& group)
{
	group.Wait();
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

		bool executed{ false };
		bool skipped{ false };
		bool failed{ false };

		try
		{
			if (task.group &&
				task.group->IsCancelRequested() &&
				task.group->GetMode() == TaskGroupMode::StopOnFirstFailure)
			{
				skipped = true;
			}
			else
			{
				task.fn(task.ctx);
				executed = true;
			}
		}

		catch (...)
		{
			failed = true;

			if (task.group)
			{
				task.group->RecordException(std::current_exception());
			}
			else
			{
				// worker를 죽이지 않고 삼킨다.
				// 추후 전역 uncaught handler / log hook 추가 가능.
			}
		}

		if (task.group)
			task.group->Release(executed, skipped, failed);
	}
}