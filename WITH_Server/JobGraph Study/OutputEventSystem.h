#pragma once

#include <iostream>
#include "ECS.h"
#include "System.h"

class OutputEventSystem : public System {
	using EventHandler = std::function<void(const OutputEvent&)>;

public:
	OutputEventSystem(ECS& ecs, int p = 0);
	virtual ~OutputEventSystem() = default;

	virtual void Execute(const float dT) override;

	virtual std::vector<std::type_index> ReadComponents() const
	{
		return { typeid(Transform), typeid(Velocity), typeid(AnimationState),
		typeid(AnimationRef), typeid(Animator), typeid(Collider) };
	}

private:
	void ProcessSpawn(const OutputEvent& event);
	void ProcessMove(const OutputEvent& event);

	std::unordered_map<DirtyType, EventHandler> _handlers;
};

// TODO List
//
// 1. 시야처리, 공간분할 통합 필요