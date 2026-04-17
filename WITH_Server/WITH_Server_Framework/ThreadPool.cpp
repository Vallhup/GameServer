#include "pch.h"
#include "ThreadPool.h"

#include <algorithm>
#include <stdexcept>
#include <string>

ThreadPool::~ThreadPool()
{
	Stop();
}

bool ThreadPool::Start(uint32_t threadCnt, WorkerPumpFn pump, void* pumpCtx)
{
	if (pump == nullptr)
		return false;

	bool expected{ false };
	if (!_running.compare_exchange_strong(expected, true))
	{
		return false;
	}

	if (threadCnt == 0)
		threadCnt = std::max(1u, std::thread::hardware_concurrency());

	_stopping.store(false);
	_pump = pump;
	_pumpCtx = pumpCtx;

	_workers.clear();
	_workers.reserve(threadCnt);

	try
	{
		for (uint32_t i = 0; i < threadCnt; ++i)
		{
			_workers.emplace_back([this]() { WorkerLoop(); });
		}
	}
	catch (...)
	{
		_stopping.store(true);
		_cv.notify_all();

		for (std::thread& worker : _workers)
		{
			if (worker.joinable())
				worker.join();
		}
		_workers.clear();

		_pump = nullptr;
		_pumpCtx = nullptr;
		_running.store(false);
		throw;
	}

	return true;
}

void ThreadPool::Stop() noexcept
{
    bool expected = true;
    if (!_running.compare_exchange_strong(expected, false))
    {
        return;
    }

    _stopping.store(true);
    _cv.notify_all();

    for (std::thread& worker : _workers)
    {
        if (worker.joinable())
            worker.join();
    }

    _workers.clear();
    _pump = nullptr;
    _pumpCtx = nullptr;
}

void ThreadPool::WakeOne() noexcept
{
    if (!_running.load())
        return;

    _workEpoch.fetch_add(1);
    _cv.notify_one();
}

void ThreadPool::WakeAll() noexcept
{
    if (!_running.load())
        return;

    _workEpoch.fetch_add(1);
    _cv.notify_all();
}

void ThreadPool::WorkerLoop()
{
    uint64_t observedEpoch = _workEpoch.load();

    while (true)
    {
        // work가 있는 동안 최대한 drain
        while (!_stopping.load())
        {
            WorkerPumpFn pump = _pump;
            void* pumpCtx = _pumpCtx;

            if (pump == nullptr)
                break;

            bool executed{ false };
            try
            {
                executed = pump(pumpCtx);
            }
            catch (...)
            {
                // pool은 정책을 모른다.
                // executor가 node state를 통해 실패를 정규화해야 하므로
                // 여기서는 worker를 유지한다.
                executed = false;
            }

            if (!executed)
                break;
        }

        std::unique_lock lock{ _cvMtx };
        _cv.wait(lock, 
            [&]()
            {
                return 
                    _stopping.load() ||
                    _workEpoch.load() != observedEpoch;
            });

        if (_stopping.load())
            return;

        observedEpoch = _workEpoch.load();
    }
}