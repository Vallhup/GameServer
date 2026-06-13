#include "pch.h"
#include "ServerReplicationSnapshot.h"

#include <algorithm>

#include "RepComponent.h"
#include "ServerPacketStager.h"
#include "WorldDef.h"
#include "WorldInstance.h"

namespace
{
	bool IsExcludedNetId(
		NetId netId,
		std::span<const NetId> excludedNetIds) noexcept
	{
		return std::find(
			excludedNetIds.begin(),
			excludedNetIds.end(),
			netId) != excludedNetIds.end();
	}
}

bool ServerReplicationSnapshot::TryGetReplicatedSpawnState(
	FrameworkRuntime& framework,
	WorldId worldId,
	Entity entity,
	CharacterId& outCharacterId,
	const WorldTransformComp*& outTransform)
{
	WorldInstance* const world = framework.FindWorld(worldId);
	if (world == nullptr)
	{
		return false;
	}

	ECSView view = world->GetRuntime().MakeView();
	if (!view.HasComponent<ReplicatedTag>(entity))
	{
		return false;
	}

	const SpawnTypeComp* const spawnType =
		view.GetComponent<SpawnTypeComp>(entity);
	if (spawnType == nullptr)
	{
		return false;
	}

	outCharacterId = spawnType->characterId;
	outTransform = view.GetComponent<WorldTransformComp>(entity);
	return true;
}

bool ServerReplicationSnapshot::TryGetPlazaPlayerTitleState(
	FrameworkRuntime& framework,
	WorldId worldId,
	Entity entity,
	TitleId& outTitleId)
{
	outTitleId = InvalidTitleId;

	WorldInstance* const world = framework.FindWorld(worldId);
	const WorldDef* const worldDef =
		world != nullptr ? world->GetDef() : nullptr;
	if (world == nullptr ||
		worldDef == nullptr ||
		worldDef->id != WorldDefId::Plaza)
	{
		return false;
	}

	ECSView view = world->GetRuntime().MakeView();
	if (view.GetComponent<PlayerControlIdentityComp>(entity) == nullptr)
	{
		return false;
	}

	const EquippedTitleStateComp* const title =
		view.GetComponent<EquippedTitleStateComp>(entity);
	if (title == nullptr)
	{
		return false;
	}

	outTitleId = title->titleId;
	return true;
}

void ServerReplicationSnapshot::StageExistingWorldEntitiesForSession(
	FrameworkRuntime& framework,
	NetworkRuntime& network,
	WorldId worldId,
	SessionId sessionId,
	NetId excludedNetId)
{
	if (!excludedNetId.IsValid())
	{
		StageExistingWorldEntitiesForSession(
			framework,
			network,
			worldId,
			sessionId,
			std::span<const NetId>());
		return;
	}

	const NetId excludedNetIds[] = { excludedNetId };
	StageExistingWorldEntitiesForSession(
		framework,
		network,
		worldId,
		sessionId,
		std::span<const NetId>(excludedNetIds, 1));
}

void ServerReplicationSnapshot::StageExistingWorldEntitiesForSession(
	FrameworkRuntime& framework,
	NetworkRuntime& network,
	WorldId worldId,
	SessionId sessionId,
	std::span<const NetId> excludedNetIds)
{
	WorldInstance* const world = framework.FindWorld(worldId);
	if (world == nullptr)
	{
		return;
	}

	ECSView view = world->GetRuntime().MakeView();
	for (auto [entity, spawnType] : view.View<SpawnTypeComp>())
	{
		if (!view.HasComponent<ReplicatedTag>(entity))
		{
			continue;
		}

		const NetId entityNetId = framework.FindNetId(worldId, entity);
		if (!entityNetId.IsValid() ||
			IsExcludedNetId(entityNetId, excludedNetIds))
		{
			continue;
		}

		const WorldTransformComp* const transform =
			view.GetComponent<WorldTransformComp>(entity);
		(void)ServerPacketStager::StageSpawnAddPacketToSession(
			network,
			sessionId,
			entityNetId,
			spawnType.characterId,
			transform);

		TitleId equippedTitleId{ InvalidTitleId };
		if (TryGetPlazaPlayerTitleState(
			framework,
			worldId,
			entity,
			equippedTitleId))
		{
			(void)ServerPacketStager::StageTitleReplicationPacketToSession(
				network,
				sessionId,
				entityNetId,
				equippedTitleId);
		}

		const AnimationPlaybackStateComp* const playback =
			view.GetComponent<AnimationPlaybackStateComp>(entity);
		if (playback != nullptr &&
			playback->animationId != AnimationId::None)
		{
			(void)ServerPacketStager::StageAnimationPacketToSession(
				network,
				sessionId,
				entityNetId,
				*playback);
		}

		const CombatStatStateComp* const stats =
			view.GetComponent<CombatStatStateComp>(entity);
		if (stats != nullptr)
		{
			(void)ServerPacketStager::StageStatPacketToSession(
				network,
				sessionId,
				entityNetId,
				*stats);
		}

		const GameplayEffectStateComp* const effects =
			view.GetComponent<GameplayEffectStateComp>(entity);
		const GameplayEffectReplicationComp* const effectReplication =
			view.GetComponent<GameplayEffectReplicationComp>(entity);
		if (effects != nullptr && effectReplication != nullptr)
		{
			(void)ServerPacketStager::StageGameplayEffectPacketToSession(
				network,
				sessionId,
				entityNetId,
				*effects,
				*effectReplication,
				false);
		}

		const AIDecisionComp* const decision =
			view.GetComponent<AIDecisionComp>(entity);
		if (decision != nullptr)
		{
			(void)ServerPacketStager::StageMonsterCombatStatePacketToSession(
				network,
				sessionId,
				entityNetId,
				IsMonsterCombatAIState(decision->curState));
		}

		const PlayerControlIdentityComp* const player =
			view.GetComponent<PlayerControlIdentityComp>(entity);
		const ConsumableInventoryComp* const inventory =
			view.GetComponent<ConsumableInventoryComp>(entity);
		if (player != nullptr &&
			inventory != nullptr &&
			player->ownerSessionId == sessionId)
		{
			(void)ServerPacketStager::StageItemCountPacketToSession(
				network,
				sessionId,
				*inventory);
		}
	}
}
