#include "pch.h"
#include "CollectReplicationTodoSourceSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const StaticSystemMetaStorage<2> CollectReplicationTodoSourceSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<CollectReplicationTodoSourceSystem>(),
		"CollectReplicationTodoSourceSystem",
		std::array<AccessSpec, 2>
	{
		WriteImmediate(ComponentRes<PendingProjectileSpawnComp>()),
		WriteImmediate(ComponentRes<PendingAbilityPresentationEventComp>()),
	});

void CollectReplicationTodoSourceSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, projectile] :
		ctx.ecs.View<PendingProjectileSpawnComp>())
	{
		(void)entity;
		projectile.requests.clear();
	}

	for (auto [entity, presentation] :
		ctx.ecs.View<PendingAbilityPresentationEventComp>())
	{
		(void)entity;
		presentation.events.clear();
	}
}
