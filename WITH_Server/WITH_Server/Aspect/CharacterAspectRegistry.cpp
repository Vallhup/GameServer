#include "pch.h"
#include "CharacterAspectRegistry.h"

#include "AIControlAspect.h"
#include "BaseCombatantAspect.h"
#include "BossPhaseAspect.h"
#include "CombatHitAspect.h"
#include "IdentityAspect.h"
#include "LocomotionPhysicsAspect.h"
#include "PlayerControlAspect.h"
#include "PortalAspect.h"
#include "ReplicationAspect.h"

namespace
{
	bool DefMatches(
		const CharacterDef& def,
		CharacterFeatureFlags required) noexcept
	{
		if (required == CharacterFeatureFlags::None)
		{
			return true;
		}
		return def.HasFeature(required);
	}
}

void CharacterAspectRegistry::Add(
	std::unique_ptr<ICharacterAspect> aspect)
{
	if (aspect != nullptr)
	{
		_aspects.push_back(std::move(aspect));
	}
}

CharacterAspectRegistry CharacterAspectRegistry::BuildDefault()
{
	CharacterAspectRegistry registry;
	registry.Add(std::make_unique<IdentityAspect>());
	registry.Add(std::make_unique<ReplicationAspect>());
	registry.Add(std::make_unique<BaseCombatantAspect>());
	registry.Add(std::make_unique<LocomotionPhysicsAspect>());
	registry.Add(std::make_unique<CombatHitAspect>());
	registry.Add(std::make_unique<PortalAspect>());
	registry.Add(std::make_unique<PlayerControlAspect>());
	registry.Add(std::make_unique<AIControlAspect>());
	registry.Add(std::make_unique<BossPhaseAspect>());
	return registry;
}

void CharacterAspectRegistry::RegisterStoragesAll(
	WorldRuntime& runtime) const
{
	for (const auto& aspect : _aspects)
	{
		aspect->RegisterStorages(runtime);
	}
}

void CharacterAspectRegistry::Assemble(
	WorldRuntime& runtime,
	Entity entity,
	const CharacterDef& def,
	const AssembleParams& params) const
{
	for (const auto& aspect : _aspects)
	{
		if (DefMatches(def, aspect->RequiredFeature()))
		{
			aspect->Attach(runtime, entity, def, params);
		}
	}
}

bool CharacterAspectRegistry::ValidateAll(
	const CharacterDef& def,
	std::string& outError) const
{
	for (const auto& aspect : _aspects)
	{
		if (!DefMatches(def, aspect->RequiredFeature()))
		{
			continue;
		}
		if (!aspect->Validate(def, outError))
		{
			return false;
		}
	}
	return true;
}

const CharacterAspectRegistry& GetGlobalCharacterAspectRegistry()
{
	static const CharacterAspectRegistry instance =
		CharacterAspectRegistry::BuildDefault();
	return instance;
}
