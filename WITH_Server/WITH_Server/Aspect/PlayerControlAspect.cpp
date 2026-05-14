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
	runtime.RegisterStorage<ConsumableInventoryComp>();
}

void PlayerControlAspect::Attach(
	WorldRuntime& runtime,
	Entity entity,
	const CharacterDef& def,
	const AssembleParams& params) const
{
	runtime.DeferredUpsertComponent<PlayerControlIdentityComp>(entity,
		PlayerControlIdentityComp
		{
			.netId			= params.netId,
			.ownerSessionId = params.sessionId.value_or(0)
		});

	const HpPotionTuning potionTuning{};
	runtime.DeferredUpsertComponent<ConsumableInventoryComp>(
		entity,
		ConsumableInventoryComp{
			.hpPotionCount = potionTuning.defaultGrantCount
		});
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
