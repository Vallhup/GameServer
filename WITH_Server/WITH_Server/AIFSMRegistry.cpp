#include "pch.h"
#include "AIFSMRegistry.h"

#include "NormalAIIdleState.h"
#include "NormalAIChaseState.h"
#include "NormalAICombatState.h"
#include "NormalAISearchState.h"
#include "NormalAIReturnHomeState.h"
#include "NormalAIReactState.h"

#include "NormalAIMovementPolicy.h"
#include "BossAIMovementPolicy.h"
#include "BossCombatActionPolicy.h"
#include "ImpCombatActionPolicy.h"
#include "WeightedCombatActionPolicy.h"
#include "NormalReactionPolicy.h"
#include "BossReactionPolicy.h"

#include <stdexcept>

namespace
{
	static const AIBehaviorProfileDef& GetRequiredProfile(
		AIArchetype aiType,
		AITuningId aiTuningId)
	{
		if (const AIBehaviorProfileDef* profile =
			FindAIBehaviorProfileDef(aiType, aiTuningId))
		{
			return *profile;
		}

		throw std::runtime_error("Required AI behavior profile was not loaded.");
	}
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
	switch (profileDef.movementPolicyKind) {
	case AIMovementPolicyKind::Normal:
	{
		movementPolicy = std::make_unique<NormalAIMovementPolicy>();
		break;
	}
	case AIMovementPolicyKind::BossPattern:
	{
		movementPolicy = std::make_unique<BossAIMovementPolicy>();
		break;
	}
	default:
	{
		break;
	}
	}

	switch (profileDef.combatActionPolicyKind) {
	case AICombatActionPolicyKind::Imp:
	{
		combatActionPolicy = std::make_unique<ImpCombatActionPolicy>();
		break;
	}
	case AICombatActionPolicyKind::Weighted:
	{
		combatActionPolicy = std::make_unique<WeightedCombatActionPolicy>();
		break;
	}
	case AICombatActionPolicyKind::BossPattern:
	{
		combatActionPolicy = std::make_unique<BossCombatActionPolicy>();
		break;
	}
	default:
	{
		break;
	}
	}

	switch (profileDef.reactionPolicyKind) {
	case AIReactionPolicyKind::Normal:
	{
		reactionPolicy = std::make_unique<NormalReactionPolicy>();
		break;
	}
	case AIReactionPolicyKind::BossPattern:
	{
		reactionPolicy = std::make_unique<BossReactionPolicy>();
		break;
	}
	default:
	{
		break;
	}
	}
}

/* --------------[ AIFSMRegistry ]-------------- */

AIFSMRegistry::AIFSMRegistry()
	: _normal(AIFSMBundle(AIArchetype::NormalMonster))
	, _firstBoss(AIFSMBundle(AIArchetype::FirstBossMonster))
	, _impBehavior(GetRequiredProfile(AIArchetype::NormalMonster, AITuningIds::Imp))
	, _demonStrikerBehavior(GetRequiredProfile(AIArchetype::NormalMonster, AITuningIds::DemonStriker))
	, _demonExecutionerBehavior(GetRequiredProfile(AIArchetype::NormalMonster, AITuningIds::DemonExecutioner))
	, _bigDemonWarriorBehavior(GetRequiredProfile(AIArchetype::FirstBossMonster, AITuningIds::BigDemonWarrior))
{
}

const AIFSMBundle* AIFSMRegistry::TryGetBundle(AIArchetype type) const
{
	switch (type) {
	case AIArchetype::NormalMonster:
	{
		return &_normal;
	}
	case AIArchetype::FirstBossMonster:
	{
		return &_firstBoss;
	}
	default:
	{
		return nullptr;
	}
	}
}

const AIBehaviorBundle* AIFSMRegistry::TryGetBehavior(
	AIArchetype aiType,
	AITuningId aiTuningId) const
{
	switch (aiType){
	case AIArchetype::NormalMonster:
	{
		if (aiTuningId == AITuningIds::Imp)
			return &_impBehavior;

		else if (aiTuningId == AITuningIds::DemonStriker)
			return &_demonStrikerBehavior;

		else if (aiTuningId == AITuningIds::DemonExecutioner)
			return &_demonExecutionerBehavior;

		return nullptr;
	}
	case AIArchetype::FirstBossMonster:
	{
		if (aiTuningId == AITuningIds::BigDemonWarrior)
			return &_bigDemonWarriorBehavior;

		return nullptr;
	}
	default:
	{
		return nullptr;
	}
	}
}

bool AIFSMRegistry::IsArchetypeSupported(AIArchetype type) noexcept
{
	switch (type) {
	case AIArchetype::NormalMonster:
	case AIArchetype::FirstBossMonster:
	{
		return true;
	}
	default:
	{
		return false;
	}
	}
}

bool AIFSMRegistry::IsBehaviorSupported(
	AIArchetype aiType,
	AITuningId aiTuningId) noexcept
{
	if (aiType != AIArchetype::NormalMonster)
	{
		return
			aiType == AIArchetype::FirstBossMonster &&
			aiTuningId == AITuningIds::BigDemonWarrior;
	}

	return
		aiTuningId == AITuningIds::Imp ||
		aiTuningId == AITuningIds::DemonStriker ||
		aiTuningId == AITuningIds::DemonExecutioner;
}
