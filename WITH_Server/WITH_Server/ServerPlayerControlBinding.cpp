#include "pch.h"
#include "ServerPlayerControlBinding.h"

#include "ECS/GameplayRuntimeComponents.h"
#include "WorldInstance.h"

bool ServerPlayerControlBinding::AssignPlayerControlNetId(
	FrameworkRuntime& framework,
	WorldId worldId,
	Entity entity,
	NetId netId)
{
	WorldInstance* const world = framework.FindWorld(worldId);
	if (world == nullptr)
	{
		return false;
	}

	ECSView view = world->GetRuntime().MakeView();
	auto* identity = const_cast<PlayerControlIdentityComp*>(
		view.GetComponent<PlayerControlIdentityComp>(entity));
	if (identity == nullptr)
	{
		return false;
	}

	identity->netId = netId;
	return true;
}
