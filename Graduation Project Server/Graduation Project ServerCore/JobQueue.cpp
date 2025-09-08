#include "pch.h"
#include "JobQueue.h"

void JobQueue::Push(const std::shared_ptr<Job>& job)
{
	_jobs.push(job);
}

bool JobQueue::TryPop(std::shared_ptr<Job>& out)
{
	return _jobs.try_pop(out);
}
