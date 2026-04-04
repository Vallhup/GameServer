#include "pch.h"
#include "FinalizePostCommitStateSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta FinalizePostCommitStateSystem::kMeta =
	MakeSystemMeta<FinalizePostCommitStateSystem>(
		"FinalizePostCommitStateSystem");

void FinalizePostCommitStateSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, result] : ctx.ecs.View<PendingCombatResultComp>())
	{
		(void)entity;
		result = {};
	}
}

const SystemMeta& FinalizePostCommitStateSystem::Meta() const
{
	return kMeta;
}
