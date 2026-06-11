#pragma once

#include "ECS/Components/GameplayWorldLifecycleComponents.h"
#include "WorldContentIds.h"

namespace PlayerDeathStatePolicy
{
	inline bool IsRespawnWorld(WorldDefId worldDefId) noexcept
	{
		return
			worldDefId == WorldDefId::Village ||
			worldDefId == WorldDefId::Castle ||
			worldDefId == WorldDefId::Final;
	}

	inline bool CanLatchRespawnRequest(
		const PlayerDeathStateComp& deathState) noexcept
	{
		return deathState.state == PlayerDeathState::AwaitingRespawnInput;
	}

	inline bool ShouldReturnExhaustedPartyToPlaza(
		bool deathCountExhausted,
		uint32_t partyMemberCount,
		uint32_t aliveMemberCount) noexcept
	{
		return
			deathCountExhausted &&
			partyMemberCount > 0 &&
			aliveMemberCount == 0;
	}

	inline bool ApplyDeathCountDecision(
		PlayerDeathStateComp& deathState,
		bool canRespawn,
		uint64_t deathCountRevision) noexcept
	{
		if (deathState.state != PlayerDeathState::WaitingForDeathCount)
		{
			return false;
		}

		deathState.state = canRespawn
			? PlayerDeathState::AwaitingRespawnInput
			: PlayerDeathState::DeathCountExhausted;
		deathState.deathCountRevision = deathCountRevision;
		deathState.respawnRequested = false;
		return true;
	}
}
