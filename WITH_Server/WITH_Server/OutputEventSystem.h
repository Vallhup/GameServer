#pragma once

#include "ECS.h"
#include "System.h"

class OutputEventSystem : public System {
	using EventHandler = std::function<void(const OutputEvent&)>;

public:
	OutputEventSystem(ECS& ecs, int p = 0);
	virtual ~OutputEventSystem() = default;

	virtual void Execute(const float dT) override;

private:
	void ProcessSpawn(const OutputEvent& event);
	void ProcessDespawn(const OutputEvent& event);
	void ProcessMove(const OutputEvent& event);

	void EnqueueToSession(uint32 sid, const void* data, uint32 len);
	void FlushOne(uint32 sid, SendBuffer*& buffer);
	void FlushAll();

	std::unordered_map<DirtyType, EventHandler> _handlers;
	std::vector<SendBuffer*> _outBuffers;
	//std::unordered_map<uint32, SendBuffer*> _outBuffers;
};

// TODO List
//
// 1. 시야처리, 공간분할 통합 필요