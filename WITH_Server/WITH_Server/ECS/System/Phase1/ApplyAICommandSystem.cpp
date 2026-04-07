#include "pch.h"
#include "ApplyAICommandSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta ApplyAICommandSystem::kMeta =
	MakeSystemMeta<ApplyAICommandSystem>("ApplyAICommandSystem");

void ApplyAICommandSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, _, frame, input] :
		ctx.ecs.View<AIControlledTag, AICommandFrameComp, ActorInputComp>())
	{
		auto* mutableInput = MutableComponent<ActorInputComp>(ctx.ecs, entity);
		if (mutableInput == nullptr)
		{
			continue;
		}

		if (frame.hasMove)
		{
			mutableInput->move.inputX       = frame.moveX;
			mutableInput->move.inputZ       = frame.moveZ;
			mutableInput->move.cameraYawRad = frame.moveYaw;
			mutableInput->move.wantsRun     = frame.wantsRun;
			mutableInput->move.lastUpdatedFrame = ctx.runtime.FrameIndex();
		}

		if (frame.hasAction)
		{
			mutableInput->action.directActionId = frame.actionId;
			mutableInput->action.directionX     = frame.actionDirX;
			mutableInput->action.directionZ     = frame.actionDirZ;
			mutableInput->action.requestedFrame = ctx.runtime.FrameIndex();
		}
	}
}

const SystemMeta& ApplyAICommandSystem::Meta() const
{
	return kMeta;
}
