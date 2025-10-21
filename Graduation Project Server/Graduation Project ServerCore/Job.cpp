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

//void ObjectBatchJob::Execute()
//{
//	auto list = _instance->GetGameObjectList();
//	for (size_t i = _start; i < _end && i < list.size(); ++i) {
//		list[i]->LogicUpdate(_deltaTime);
//	}
//	// 완료 처리
//	if (--_instance->_pendingBatches == 0) {
//		_instance->GetJobQueue().Push(new FinalizeJob(_instance));
//	}
//}
//
//void FinalizeJob::Execute()
//{
//	_inst->GetGameLogic().NetworkUpdate(); // 이번 틱 결과 전송
//	_inst->_isUpdating.store(false);       // 다음 틱 허용
//}