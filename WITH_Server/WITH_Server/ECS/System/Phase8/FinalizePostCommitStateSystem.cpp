#include "pch.h"
#include "FinalizePostCommitStateSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const StaticSystemMetaStorage<1> FinalizePostCommitStateSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<FinalizePostCommitStateSystem>(),
		"FinalizePostCommitStateSystem",
		std::array<AccessSpec, 1>
	{
		WriteImmediate(ComponentRes<PendingCombatResultComp>()),
	});

void FinalizePostCommitStateSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, result] : ctx.ecs.View<PendingCombatResultComp>())
	{
		(void)entity;
		result = {};
	}
}
