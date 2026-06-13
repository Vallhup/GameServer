#pragma once

#include <span>

#include "CharacterDef.h"
#include "Entity.h"
#include "FrameworkRuntime.h"
#include "NetworkRuntime.h"
#include "Session.h"
#include "TitleDef.h"
#include "WorldId.h"
#include "ECS/GameplayRuntimeComponents.h"

class ServerReplicationSnapshot final {
public:
	static bool TryGetReplicatedSpawnState(
		FrameworkRuntime& framework,
		WorldId worldId,
		Entity entity,
		CharacterId& outCharacterId,
		const WorldTransformComp*& outTransform);

	static bool TryGetPlazaPlayerTitleState(
		FrameworkRuntime& framework,
		WorldId worldId,
		Entity entity,
		TitleId& outTitleId);

	static void StageExistingWorldEntitiesForSession(
		FrameworkRuntime& framework,
		NetworkRuntime& network,
		WorldId worldId,
		SessionId sessionId,
		NetId excludedNetId = NetId::Invalid());

	static void StageExistingWorldEntitiesForSession(
		FrameworkRuntime& framework,
		NetworkRuntime& network,
		WorldId worldId,
		SessionId sessionId,
		std::span<const NetId> excludedNetIds);
};
