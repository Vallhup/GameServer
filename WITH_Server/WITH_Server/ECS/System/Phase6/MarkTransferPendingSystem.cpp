#include "pch.h"
#include "MarkTransferPendingSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	const std::array<AccessSpec, 2> kMarkTransferPendingAccesses{
		WriteImmediate(ComponentRes<PendingWorldTransferComp>()),
		ReadSnapshot(ComponentRes<PendingWorldTransferTag>()),
	};
}

const SystemMeta MarkTransferPendingSystem::kMeta =
	SystemMeta{
		SysTag<MarkTransferPendingSystem>(),
		"MarkTransferPendingSystem",
		kMarkTransferPendingAccesses,
		kNoDeps,
		kNoDeps
	};

void MarkTransferPendingSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, transfer, tag] :
		ctx.ecs.View<PendingWorldTransferComp, PendingWorldTransferTag>())
	{
		(void)entity;
		(void)tag;
		if (transfer.requestedFrameIndex == 0)
		{
			transfer.requestedFrameIndex = ctx.runtime.FrameIndex();
		}
	}
}

const SystemMeta& MarkTransferPendingSystem::Meta() const
{
	return kMeta;
}
