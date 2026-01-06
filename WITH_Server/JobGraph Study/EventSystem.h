#pragma once

#include "ECS.h"
#include "System.h"

class EventSystem : public System {
	using EventHandler = std::function<void(const Event&)>;

public:
	EventSystem(ECS& e, int p = 0);

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> WriteComponents() const 
	{ 
		return { typeid(Transform), typeid(Velocity), typeid(AnimationState), 
		typeid(AnimationRef), typeid(Animator), typeid(Collider) };
	}

private:
	void ProcessConnect(const Event& event);
	void ProcessDisconnect(const Event& event);
	void ProcessMove(const Event& event);
	void ProcessAction(const Event& event);

	std::unordered_map<EventType, EventHandler> _handlers;
};