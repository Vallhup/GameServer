#include "pch.h"
#include "AIFSMRegistry.h"

#include "AIIdleState.h"
#include "AIChaseState.h"
#include "AICombatState.h"
#include "AISearchState.h"
#include "AIReturnHomeState.h"
#include "AIReactState.h"

#include "DataDrivenAIMovementPolicy.h"
#include "DataDrivenAIIdleActionPolicy.h"
#include "DataDrivenAICombatActionPolicy.h"
#include "DataDrivenAIReactionPolicy.h"
#include "DataDrivenAISpecialActionPolicy.h"

#include "GameDataCatalog.h"

/* --------------[ AIStateRegistry ]-------------- */

AIStateRegistry::AIStateRegistry()
{
	_states[static_cast<size_t>(AIStateType::Idle)]			= std::make_unique<AIIdleState>();
	_states[static_cast<size_t>(AIStateType::Chase)]		= std::make_unique<AIChaseState>();
	_states[static_cast<size_t>(AIStateType::Combat)]		= std::make_unique<AICombatState>();
	_states[static_cast<size_t>(AIStateType::Search)]		= std::make_unique<AISearchState>();
	_states[static_cast<size_t>(AIStateType::ReturnHome)]	= std::make_unique<AIReturnHomeState>();
	_states[static_cast<size_t>(AIStateType::React)]		= std::make_unique<AIReactState>();
}

const IAIState* AIStateRegistry::TryGetState(AIStateType type) const
{
	const size_t index = static_cast<size_t>(type);

	if (index >= _states.size())
		return nullptr;

	return _states[index].get();
}

/* --------------[ AIBehaviorBundle ]-------------- */

AIBehaviorBundle::AIBehaviorBundle(const AIBehaviorProfileDef& profileDef)
	: profile(&profileDef)
{
	movementPolicy		= std::make_unique<DataDrivenAIMovementPolicy>();
	idleActionPolicy	= std::make_unique<DataDrivenAIIdleActionPolicy>();
	combatActionPolicy	= std::make_unique<DataDrivenAICombatActionPolicy>();
	reactionPolicy		= std::make_unique<DataDrivenAIReactionPolicy>();
	specialActionPolicy = std::make_unique<DataDrivenAISpecialActionPolicy>();
}

/* --------------[ AIFSMRegistry ]-------------- */

AIFSMRegistry::AIFSMRegistry()
	: AIFSMRegistry(GameDataCatalog::Current().AIBehaviors().GetAll())
{
}

AIFSMRegistry::AIFSMRegistry(std::span<const AIBehaviorProfileDef> profiles)
{
	Build(profiles);
}

const AIFSMBundle* AIFSMRegistry::TryGetBundle(AIArchetype type) const
{
	const auto it = _bundles.find(type);
	if (it == _bundles.end()) return nullptr;
	return &it->second;
}

const AIBehaviorBundle* AIFSMRegistry::TryGetBehavior(
	AIBehaviorProfileId profileId) const
{
	const auto it = _behaviors.find(profileId);
	if (it == _behaviors.end()) return nullptr;
	return &it->second;
}

bool AIFSMRegistry::IsArchetypeSupported(AIArchetype type) noexcept
{
	switch (type) {
	case AIArchetype::NormalMonster:
	case AIArchetype::FirstBossMonster:
	case AIArchetype::MidBossMonster:
	case AIArchetype::FinalBossMonster:
	{
		return true;
	}
	default:
	{
		return false;
	}
	}
}

bool AIFSMRegistry::IsBehaviorProfileSupported(
	const AIBehaviorProfileDef& profile) noexcept
{
	return
		IsArchetypeSupported(profile.aiType);
}

bool AIFSMRegistry::IsBehaviorSupported(
	AIBehaviorProfileId profileId) noexcept
{
	const GameDataCatalog* const catalog = GameDataCatalog::TryCurrent();
	if (catalog == nullptr)
		return false;

	const AIBehaviorProfileDef* const profile =
		catalog->AIBehaviors().Find(profileId);
	if (profile == nullptr)
		return false;

	return IsBehaviorProfileSupported(*profile);
}

void AIFSMRegistry::Build(std::span<const AIBehaviorProfileDef> profiles)
{
	_bundles.clear();
	_behaviors.clear();

	for (const AIBehaviorProfileDef& profile : profiles)
	{
		if (!IsBehaviorProfileSupported(profile))
			continue;

		_bundles.try_emplace(profile.aiType, profile.aiType);
		_behaviors.try_emplace(profile.id, profile);
	}
}
