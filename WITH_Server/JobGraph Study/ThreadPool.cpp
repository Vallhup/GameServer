#include "ThreadPool.h"
#include "JobGraph.h"
#include "Network.h"

ThreadPool::ThreadPool(size_t size) : _running(true)
{
	for (size_t i = 0; i < size; ++i)
		_workers.emplace_back(&ThreadPool::WorkerLoop, this);
}

void ThreadPool::Push(const JobData& job)
{
	{
		std::lock_guard lock{ _mtx };
		_jobQueue.push(job);
	}

	_cv.notify_one();
}

void ThreadPool::Stop()
{
	_running = false;
	_cv.notify_all();
	for (auto& w : _workers) {
		if (w.joinable())
			w.join();
	}
}

void ThreadPool::ParallelFor(int count, int chunk, const std::function<void(int, int)>& func)
{
	int jobCount = (count + chunk - 1) / chunk;

	std::latch done(jobCount);
	for (int i = 0; i < jobCount; ++i)
	{
		int begin = i * chunk;
		int end = std::min(begin + chunk, count);

		Job* job = new ParallelForJob{
			begin, end,
			func, &done
		};

		JobData jd{ &ParallelForJob::Execute, job };
		Push(jd);
	}

	done.wait();
}

void ThreadPool::WorkerLoop()
{
	JobData job;
	while (_running) {
		{
			std::unique_lock lock{ _mtx };
			_cv.wait(lock, 
				[&]() { return not _jobQueue.empty() or not _running; });

			if (not _running) break;

			job = std::move(_jobQueue.front());
			_jobQueue.pop();
		}

		job.func(job.context);
	}
}
