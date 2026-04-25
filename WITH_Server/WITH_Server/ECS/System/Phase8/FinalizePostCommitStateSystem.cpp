#include "pch.h"
#include "FinalizePostCommitStateSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	const std::array<AccessSpec, 1> kFinalizePostCommitStateAccesses{
		WriteImmediate(ComponentRes<PendingCombatResultComp>()),
	};
}

const SystemMeta FinalizePostCommitStateSystem::kMeta =
	SystemMeta{
		SysTag<FinalizePostCommitStateSystem>(),
		"FinalizePostCommitStateSystem",
		kFinalizePostCommitStateAccesses,
		kNoDeps,
		kNoDeps
	};

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
