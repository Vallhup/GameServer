#include "pch.h"
#include "Job.h"

/*---------------[ TimerJob ]---------------*/

TimerJob::TimerJob(const std::function<void()>& f, std::chrono::high_resolution_clock::time_point time)
	: Job(time), func(f)
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

LogicJob::LogicJob(const std::shared_ptr<Instance>& i, float dT) : Job(), instance(i), deltaTime(dT)
{
}

void LogicJob::Execute()
{
	if (auto inst = instance.lock()) {
		inst->Update(deltaTime);
	}
}

/*---------------[ LogicUpdateJob ]---------------*/

//LogicUpdateJob::LogicUpdateJob(Instance* i, size_t s, size_t e, float dT, const std::shared_ptr<std::barrier<>>& b)
//	: Job(), instance(i), start(s), end(e), deltaTime(dT), barrier(b)
//{
//}
//
//void LogicUpdateJob::Execute()
//{
//	auto objList = instance->GetGameObjectList();
//	for (size_t i = start; i < end; ++i) {
//		objList[i]->LogicUpdate(deltaTime);
//	}
//
//	//cnt->fetch_sub(1);
//	barrier->arrive_and_wait();
//}
