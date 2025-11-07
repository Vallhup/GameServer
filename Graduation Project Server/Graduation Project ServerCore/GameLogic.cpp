#include "pch.h"
#include "GameLogic.h"

/*---------------[ GameLogic ]---------------*/

GameLogic::GameLogic(Instance* instance) : _instance(instance)
{
}

void GameLogic::LogicUpdate(float deltaTime)
{
	for (auto& obj : _instance->GetGameObjectList()) {
		obj->LogicUpdate(deltaTime);
	}
}

void GameLogic::NetworkUpdate()
{
	for (auto& obj : _instance->GetGameObjectList()) {
		obj->NetworkUpdate();
	}
}

void GameLogic::OnPlayerAction(int sessionId, Protocol::CS_INPUT_PACKET packet)
{
	if (auto gameObj = _instance->GetGameObject(sessionId)) {
		if(auto inputComp = gameObj->GetComponent<InputComponent>()) {
			inputComp->Enqueue(packet);
		}
	}
}

/*---------------[ TestLogic ]---------------*/

//TestLogic::TestLogic(Instance* instance) : _instance(instance), _logicCnt(0)
//{
//}
//
//void TestLogic::LogicUpdate(float deltaTime)
//{
//	const size_t objNum = _instance->GetGameObjectList().size();
//	if (objNum == 0) return;
//
//	const size_t threadCount = std::min(objNum, (size_t)std::thread::hardware_concurrency());
//	const size_t perThread = (objNum + threadCount - 1) / threadCount;
//
//	auto sync = std::make_shared<std::barrier<>>((long long)threadCount);
//
//	for (size_t i = 0; i < threadCount; ++i) 
//	{
//		size_t start = i * perThread;
//		size_t end = std::min(start + perThread, objNum);
//		if (start >= end) continue;
//
//		_instance->GetJobQueue().Push(new LogicUpdateJob(_instance, start, end, deltaTime, sync));
//	}
//
//	_logicCnt = threadCount;
//}
//
//void TestLogic::NetworkUpdate()
//{
//	for (auto& obj : _instance->GetGameObjectList()) {
//		obj->NetworkUpdate();
//	}
//}
//
//void TestLogic::OnPlayerAction(int sessionId, Protocol::CS_INPUT_PACKET packet)
//{
//	if (auto gameObj = _instance->GetGameObject(sessionId)) {
//		if (auto inputComp = gameObj->GetComponent<InputComponent>()) {
//			inputComp->Enqueue(packet);
//		}
//	}
//}