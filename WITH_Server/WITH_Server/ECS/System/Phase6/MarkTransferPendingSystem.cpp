#include "pch.h"
#include "MarkTransferPendingSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const StaticSystemMetaStorage<2> MarkTransferPendingSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<MarkTransferPendingSystem>(),
		"MarkTransferPendingSystem",
		std::array<AccessSpec, 2>
	{
		WriteImmediate(ComponentRes<PendingWorldTransferComp>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
	});

void MarkTransferPendingSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, transfer, tag] :
		ctx.ecs.View<PendingWorldTransferComp, PendingWorldTransferTag>())
	{
		if (transfer.requestedFrameIndex == 0)
		{
			transfer.requestedFrameIndex = ctx.runtime.FrameIndex();
		}
	}
}
