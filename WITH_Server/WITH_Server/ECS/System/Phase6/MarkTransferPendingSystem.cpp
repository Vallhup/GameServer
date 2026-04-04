#include "pch.h"
#include "MarkTransferPendingSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta MarkTransferPendingSystem::kMeta =
	MakeSystemMeta<MarkTransferPendingSystem>("MarkTransferPendingSystem");

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
