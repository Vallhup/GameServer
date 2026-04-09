#include "pch.h"
#include "PlayerControlAspect.h"

#include "../ECS/GameplayRuntimeComponents.h"
#include "RepComponent.h"
#include "WorldRuntime.h"

CharacterFeatureFlags PlayerControlAspect::RequiredFeature() const noexcept
{
	return CharacterFeatureFlags::Playable;
}

void PlayerControlAspect::RegisterStorages(WorldRuntime& runtime) const
{
	runtime.RegisterStorage<PlayerControlIdentityComp>();
}

void PlayerControlAspect::Attach(
	WorldRuntime& runtime,
	Entity entity,
	const CharacterDef& def,
	const AssembleParams& params) const
{
	(void)def;

	PlayerControlIdentityComp identity{};
	identity.netId = params.netId;
	identity.ownerSessionId = params.sessionId.value_or(0);
	runtime.DeferredUpsertComponent<PlayerControlIdentityComp>(
		entity, identity);
}

bool PlayerControlAspect::Validate(
	const CharacterDef& def,
	std::string& outError) const
{
	if (def.role != CharacterRole::Player)
	{
		outError = "Playable feature requires role == Player";
		return false;
	}
	return true;
}
