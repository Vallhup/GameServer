#include "pch.h"
#include "Job.h"

/*---------------[ TimerJob ]---------------*/

TimerJob::TimerJob(int id, const std::function<void()>& f, std::chrono::high_resolution_clock::time_point time)
	: Job(time), objectId(id), func(f)
{
}

void TimerJob::Execute()
{
	func();
}

/*---------------[ BTJob ]---------------*/

BTJob::BTJob(const std::function<NodeStatus()>& f) : Job(), func(f)
{
}

void BTJob::Execute()
{
	NodeStatus result = func();
	promise->set_value(result);
}

/*---------------[ LogicJob ]---------------*/

LogicJob::LogicJob(Instance* i, float dT) : Job(), instance(i), deltaTime(dT)
{
}

void LogicJob::Execute()
{
	if (instance) {
		instance->Update(deltaTime);
	}
}