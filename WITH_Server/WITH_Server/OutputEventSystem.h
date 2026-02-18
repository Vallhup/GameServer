#pragma once

#include "System.h"
#include "Event.h"

class OutputEventSystem : public System {
	using EventHandler = std::function<void(const OutputEvent&)>;

public:
	OutputEventSystem(WorldRuntime& rt, int p = 0);
	virtual ~OutputEventSystem() = default;

	virtual void Execute(const double dT) override;

	virtual std::vector<std::type_index> ReadResources() const override
	{
		return {  };
	}

	virtual std::vector<std::type_index> WriteResources() const override
	{
		return {  };
	}

private:
	void ProcessSpawn(const OutputEvent& event);
	void ProcessDespawn(const OutputEvent& event);
	void ProcessMove(const OutputEvent& event);
	void ProcessAnimationChange(const OutputEvent& event);

	void EnqueueToSession(uint32 sid, const void* data, uint32 len);
	void FlushOne(uint32 sid, SendBuffer*& buffer);
	void FlushAll();

	std::unordered_map<DirtyType, EventHandler> _handlers;
	std::vector<SendBuffer*> _outBuffers;
};

// TODO List
//
// 1. 시야처리, 공간분할 통합 필요