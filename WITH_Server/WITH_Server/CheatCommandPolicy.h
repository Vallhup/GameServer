#pragma once

#include "EntityId.h"
#include "WorldContentIds.h"

namespace CheatCommandPolicy
{
	inline CharacterId ResolveTeleportBoss(WorldDefId worldDefId) noexcept
	{
		switch (worldDefId)
		{
		case WorldDefId::Village:
			return CharacterId::BigDemonWarrior;
		case WorldDefId::Castle:
			return CharacterId::Tank;
		default:
			return CharacterId::None;
		}
	}

	inline CharacterId ResolveKillBoss(WorldDefId worldDefId) noexcept
	{
		switch (worldDefId)
		{
		case WorldDefId::Village:
			return CharacterId::BigDemonWarrior;
		case WorldDefId::Castle:
			return CharacterId::Tank;
		case WorldDefId::Final:
			return CharacterId::FinalBoss;
		default:
			return CharacterId::None;
		}
	}

	inline bool CanTransferToFinal(WorldDefId worldDefId) noexcept
	{
		return worldDefId != WorldDefId::None &&
			worldDefId != WorldDefId::Final;
	}
}
