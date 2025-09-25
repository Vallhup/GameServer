#pragma once

class Job;

class JobQueue {
public:
	void Push(const std::shared_ptr<Job>& job);
	bool TryPop(std::shared_ptr<Job>& out);

private:
	concurrency::concurrent_priority_queue<std::shared_ptr<Job>> _jobs;
	std::atomic<uint64_t> seq{ 0 };
};

