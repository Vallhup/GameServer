#include "pch.h"
#include "AIFSMRegistry.h"

#include "NormalAIIdleState.h"
#include "NormalAIChaseState.h"
#include "NormalAICombatState.h"
#include "NormalAISearchState.h"
#include "NormalAIReturnHomeState.h"
#include "NormalAIReactState.h"

#include "NormalAIMovementPolicy.h"
#include "ImpCombatActionPolicy.h"
#include "NormalReactionPolicy.h"

namespace
{
	const std::array<WeightedActionEntry, 4> kImpCombatActions =
	{
		WeightedActionEntry{ ActionId::Imp_melee1, 10 },
		WeightedActionEntry{ ActionId::Imp_melee3, 35 },
		WeightedActionEntry{ ActionId::Imp_melee4, 30 },
		WeightedActionEntry{ ActionId::Imp_melee5, 25 }
	};

	const std::array<WeightedActionEntry, 1> kImpIdleActions =
	{
		WeightedActionEntry{ ActionId::Imp_Jump, 100 }
	};

	const std::array<AIBehaviorProfileDef, 1> kAIBehaviorProfiles =
	{
		AIBehaviorProfileDef
		{
			.id = AITuningIds::Imp,
			.aiType = AIArchetype::NormalMonster,
			.perceptionTuning = AIPerceptionTuningComp
			{
				.sightRange = 12.0,
				.attackRange = 2.0,
				.frontDotThreshold = 0.2,
				.targetKeepBonus = 4.0,
				.lastAttackerBonus = 2.5,
				.frontBonus = 1.0,
				.switchScoreMargin = 3.0,
				.loseSightGraceTime = 1.2,
				.leashRange = 18.0,
				.hardLeashRange = 24.0,
				.leashGaugeMax = 100.0,
				.leashDrainPerSec = 20.0,
				.hardLeashDrainPerSec = 60.0,
				.leashRecoverPerSec = 35.0,
				.returnHomeArriveRange = 0.8,
				.returnHomeReaggroLockSec = 1.5,
				.returnHpRegenPerSecRatio = 0.08,
				.idleActionCooldownSec = 6.0,
				.idleActionChancePercent = 25,
				.assistRange = 6.0
			},
			.decisionTuning = AIDecisionTuningComp
			{
				.decisionInterval = 0.2,
				.attackCooldown = 1.8,
				.reactDuration = 0.5
			},
			.movementPolicyKind = AIMovementPolicyKind::Normal,
			.combatActionPolicyKind = AICombatActionPolicyKind::Imp,
			.idleActionPolicyKind = AIIdleActionPolicyKind::Weighted,
			.reactionPolicyKind = AIReactionPolicyKind::Normal,
			.combatActions = kImpCombatActions,
			.idleActions = kImpIdleActions
		}
	};

	static const AIBehaviorProfileDef& GetRequiredProfile(
		AIArchetype aiType,
		AITuningId aiTuningId) noexcept
	{
		if (const AIBehaviorProfileDef* profile =
			AIFSMRegistry::FindBehaviorProfile(aiType, aiTuningId))
		{
			return *profile;
		}

		return kAIBehaviorProfiles[0];
	}
}

const AIBehaviorProfileDef* AIFSMRegistry::FindBehaviorProfile(
	AIArchetype aiType,
	AITuningId aiTuningId) noexcept
{
	for (const AIBehaviorProfileDef& profile : kAIBehaviorProfiles)
	{
		if (profile.aiType == aiType && profile.id == aiTuningId)
			return &profile;
	}

	return nullptr;
}

/* --------------[ AIStateRegistry ]-------------- */

AIStateRegistry::AIStateRegistry(AIArchetype type)
{
	(void)type;

	_states[static_cast<size_t>(AIStateType::Idle)]			= std::make_unique<NormalAIIdleState>();
	_states[static_cast<size_t>(AIStateType::Chase)]		= std::make_unique<NormalAIChaseState>();
	_states[static_cast<size_t>(AIStateType::Combat)]		= std::make_unique<NormalAICombatState>();
	_states[static_cast<size_t>(AIStateType::Search)]		= std::make_unique<NormalAISearchState>();
	_states[static_cast<size_t>(AIStateType::ReturnHome)]	= std::make_unique<NormalAIReturnHomeState>();
	_states[static_cast<size_t>(AIStateType::React)]		= std::make_unique<NormalAIReactState>();
}

const IAIState* AIStateRegistry::TryGetState(AIStateType type) const
{
	const size_t index = static_cast<size_t>(type);

	if (index >= _states.size())
		return nullptr;

	return _states[index].get();
}

/* --------------[ AIFSMBundle ]-------------- */

AIFSMBundle::AIFSMBundle(AIArchetype type)
	: aiType(type)
	, stateRegistry(AIStateRegistry(type))
{
}

/* --------------[ AIBehaviorBundle ]-------------- */

AIBehaviorBundle::AIBehaviorBundle(const AIBehaviorProfileDef& profileDef)
	: profile(&profileDef)
{
	switch (profileDef.movementPolicyKind)
	{
	case AIMovementPolicyKind::Normal:
		movementPolicy = std::make_unique<NormalAIMovementPolicy>();
		break;
	default:
		break;
	}

	switch (profileDef.combatActionPolicyKind)
	{
	case AICombatActionPolicyKind::Imp:
		combatActionPolicy = std::make_unique<ImpCombatActionPolicy>();
		break;
	default:
		break;
	}

	switch (profileDef.reactionPolicyKind)
	{
	case AIReactionPolicyKind::Normal:
		reactionPolicy = std::make_unique<NormalReactionPolicy>();
		break;
	default:
		break;
	}
}

/* --------------[ AIFSMRegistry ]-------------- */

AIFSMRegistry::AIFSMRegistry()
	: _normal(AIFSMBundle(AIArchetype::NormalMonster))
	, _impBehavior(GetRequiredProfile(AIArchetype::NormalMonster, AITuningIds::Imp))
{
}

const AIFSMBundle* AIFSMRegistry::TryGetBundle(AIArchetype type) const
{
	switch (type)
	{
	case AIArchetype::NormalMonster:
		return &_normal;
	default:
		return nullptr;
	}
}

const AIBehaviorBundle* AIFSMRegistry::TryGetBehavior(
	AIArchetype aiType,
	AITuningId aiTuningId) const
{
	switch (aiType)
	{
	case AIArchetype::NormalMonster:
		if (aiTuningId == AITuningIds::Imp)
			return &_impBehavior;
		return nullptr;
	default:
		return nullptr;
	}
}

bool AIFSMRegistry::IsArchetypeSupported(AIArchetype type) noexcept
{
	switch (type)
	{
	case AIArchetype::NormalMonster:
		return true;
	default:
		return false;
	}
}

bool AIFSMRegistry::IsBehaviorSupported(
	AIArchetype aiType,
	AITuningId aiTuningId) noexcept
{
	return FindBehaviorProfile(aiType, aiTuningId) != nullptr;
}
