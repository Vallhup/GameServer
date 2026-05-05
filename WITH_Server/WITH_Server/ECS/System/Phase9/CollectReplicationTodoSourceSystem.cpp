#include "pch.h"
#include "CollectReplicationTodoSourceSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	const std::array<AccessSpec, 2> kCollectReplicationTodoSourceAccesses{
		WriteImmediate(ComponentRes<PendingProjectileSpawnComp>()),
		WriteImmediate(ComponentRes<PendingAbilityPresentationEventComp>()),
	};
}

const SystemMeta CollectReplicationTodoSourceSystem::kMeta =
	SystemMeta{
		SysTag<CollectReplicationTodoSourceSystem>(),
		"CollectReplicationTodoSourceSystem",
		kCollectReplicationTodoSourceAccesses,
		kNoDeps,
		kNoDeps
	};

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

const SystemMeta& CollectReplicationTodoSourceSystem::Meta() const
{
	return kMeta;
}
