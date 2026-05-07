#pragma once

#include "Entity.h"
#include "ECS/GameplayRuntimeComponents.h"

struct SystemContext;
class IAIMovementPolicy;
class IAIIdleActionPolicy;
class IAICombatActionPolicy;
class IAIReactionPolicy;
class IAISpecialActionPolicy;
struct AIBehaviorProfileDef;
struct AIPerceptionTuningDef;
struct AIDecisionTuningDef;

struct AIContext
{
	Entity							self{ Entity::Null() };

	SystemContext*					sysCtx{ nullptr };

	AIBlackboardComp*				blackboard{ nullptr };
	AIDecisionComp*					decision{ nullptr };
	AIReactionEventQueueComp*		reaction{ nullptr };
	AIIntentFrameComp*				intent{ nullptr };
	CombatStatStateComp*			stats{ nullptr };
	AIActionRuntimeComp*			actionRuntime{ nullptr };
	AIMovementRuntimeComp*			movementRuntime{ nullptr };

	const WorldTransformComp*		selfTr{ nullptr };
	const AbilityStateComp*			abilityState{ nullptr };
	const AIPerceptionComp*			perception{ nullptr };
	const AIPerceptionTuningDef*	perceptionTuning{ nullptr };
	const AIDecisionTuningDef*		decisionTuning{ nullptr };

	const IAIMovementPolicy*		movementPolicy{ nullptr };
	const IAIIdleActionPolicy*		idleActionPolicy{ nullptr };
	const IAICombatActionPolicy*	combatActionPolicy{ nullptr };
	const IAIReactionPolicy*		reactionPolicy{ nullptr };
	const IAISpecialActionPolicy*	specialActionPolicy{ nullptr };

	const AIBehaviorProfileDef*		behaviorProfile{ nullptr };
};

class IAIState {
public:
	virtual ~IAIState() = default;

	virtual AIStateType Type() const = 0;

	virtual void Enter(AIContext& ctx) const {}
	virtual void Exit(AIContext& ctx) const {}

	// Called on each decision tick.
	virtual void DecisionUpdate(AIContext& ctx, const double decisionDT) const = 0;

	// Called on each frame.
	virtual void FrameUpdate(AIContext& ctx, const double frameDT) const = 0;
};
