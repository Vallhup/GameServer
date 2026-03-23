#pragma once

#include "Entity.h"

#include "AI.h"

struct ActionState;

class WorldRutime;
class IMovementPolicy;

struct AIContext
{
	Entity self{ Entity::Null() };

	const WorldRuntime* runtime{ nullptr };

	const Transform* selfTr{ nullptr };
	const ActionState* actionState{ nullptr };
	const AIPerceptionCache* perception{ nullptr };
	const AIPerceptionTuning* perceptionTuning{ nullptr };
	const AIDecisionTuning* decisionTuning{ nullptr };

	AIBlackboard* blackboard{ nullptr };
	AIDecisionState* decision{ nullptr };
	AIReactionCache* reaction{ nullptr };
	AIIntent* intent{ nullptr };

	IMovementPolicy* movementPolicy{ nullptr };

	inline bool IsValidContext() const
	{
		return
			(self != Entity::Null()) && runtime &&
			selfTr && actionState && perception &&
			perceptionTuning && decisionTuning &&
			blackboard && decision && reaction && intent &&
			movementPolicy;
	}
};

class IAIState {
public:
	virtual ~IAIState() = default;

	virtual AIStateType Type() const = 0;

	virtual void Enter(AIContext& ctx) const {}
	virtual void Exit(AIContext& ctx) const {}
	
	// Decision Tick마다 호출
	virtual void DecisionUpdate(AIContext& ctx, const double decisionDT) const = 0;

	// 매 프레임 호출
	virtual void FrameUpdate(AIContext& ctx, const double frameDT) const = 0;
};