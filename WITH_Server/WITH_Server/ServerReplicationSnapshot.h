#pragma once

#include "CharacterDef.h"
#include "Entity.h"
#include "FrameworkRuntime.h"
#include "NetworkRuntime.h"
#include "Session.h"
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

	static void StageExistingWorldEntitiesForSession(
		FrameworkRuntime& framework,
		NetworkRuntime& network,
		WorldId worldId,
		SessionId sessionId,
		NetId excludedNetId = NetId::Invalid());
};
