#pragma once

#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <queue>

#include "JobGraph.h"

class ThreadPool {
public:
	ThreadPool(size_t size = std::thread::hardware_concurrency());
	~ThreadPool() { Stop(); }

	void Push(const JobData& job);
	void Stop();

	void ParallelFor(int count, int minPerJob, const std::function<void(int, int)>& func);

private:
	void WorkerLoop();

private:
	std::atomic<bool> _running;
	std::vector<std::thread> _workers;
	std::queue<JobData> _jobQueue;

	std::mutex _mtx;
	std::condition_variable _cv;
};