#pragma once

#include "System.h"
#include "Event.h"

class EventSystem : public System {
	using EventHandler = std::function<void(const Event&)>;

public:
	EventSystem(WorldRuntime& rt, int p = 0);

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
	void ProcessConnect(const Event& event);
	void ProcessDisconnect(const Event& event);
	void ProcessMove(const Event& event);
	void ProcessAction(const Event& event);

	std::unordered_map<EventType, EventHandler> _handlers;
};