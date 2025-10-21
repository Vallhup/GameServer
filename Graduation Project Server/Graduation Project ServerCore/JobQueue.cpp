#include "pch.h"
#include "JobQueue.h"

void JobQueue::Push(Job* job)
{
	_jobs.push(job);
}

bool JobQueue::TryPop(Job*& out)
{
	return _jobs.try_pop(out);
}
