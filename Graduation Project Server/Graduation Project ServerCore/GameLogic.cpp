#include "pch.h"
#include "GameLogic.h"

GameLogic::GameLogic(Instance* instance) : _instance(instance)
{
}

void GameLogic::LogicUpdate(float deltaTime)
{
	for (auto& obj : _instance->GetGameObjectList()) {
		obj->LogicUpdate(deltaTime);
	}

	//auto objectList = _instance->GetGameObjectList();
	//if (objectList.empty()) {
	//	_instance->_isUpdating.store(false);
	//	return;
	//}

	//// Batch 단위로 나누기
	//const size_t batchSize = 10; // 조정 가능
	//const size_t totalObjects = objectList.size();
	//const size_t numBatches = (totalObjects + batchSize - 1) / batchSize;

	//_instance->_pendingBatches.store((int)numBatches, std::memory_order_relaxed);

	//auto& jobQueue = _instance->GetJobQueue();

	//for (size_t i = 0; i < numBatches; ++i) {
	//	const size_t start = i * batchSize;
	//	const size_t end = std::min(start + batchSize, totalObjects);
	//	jobQueue.Push(new ObjectBatchJob(_instance, deltaTime, start, end));
	//}
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