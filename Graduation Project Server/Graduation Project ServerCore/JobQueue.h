#pragma once

class Job;

class JobQueue {
public:
	void Push(Job* job);
	bool TryPop(Job*& out);

private:
	concurrency::concurrent_priority_queue<Job*> _jobs;
};