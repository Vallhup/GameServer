#include "pch.h"
#include "CollectReplicationTodoSourceSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta CollectReplicationTodoSourceSystem::kMeta =
	MakeSystemMeta<CollectReplicationTodoSourceSystem>(
		"CollectReplicationTodoSourceSystem");

void CollectReplicationTodoSourceSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, projectile] :
		ctx.ecs.View<PendingProjectileSpawnComp>())
	{
		(void)entity;
		projectile.requests.clear();
	}

	for (auto [entity, presentation] :
		ctx.ecs.View<PendingActionPresentationEventComp>())
	{
		(void)entity;
		presentation.events.clear();
	}
}

const SystemMeta& CollectReplicationTodoSourceSystem::Meta() const
{
	return kMeta;
}
