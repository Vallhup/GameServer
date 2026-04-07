#pragma once

#include "Entity.h"
#include "ECS/GameplayRuntimeComponents.h"

struct SystemContext;
class IAIMovementPolicy;

struct AIContext
{
	Entity self{ Entity::Null() };

	const SystemContext* sysCtx{ nullptr };

	const WorldTransformComp* selfTr{ nullptr };
	const ActionStateComp* actionState{ nullptr };
	const AIPerceptionComp* perception{ nullptr };
	const AIPerceptionTuningComp* perceptionTuning{ nullptr };
	const AIDecisionTuningComp* decisionTuning{ nullptr };

	AIBlackboardComp* blackboard{ nullptr };
	AIDecisionComp* decision{ nullptr };
	AIReactionComp* reaction{ nullptr };
	AICommandFrameComp* command{ nullptr };

	IAIMovementPolicy* movementPolicy{ nullptr };
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