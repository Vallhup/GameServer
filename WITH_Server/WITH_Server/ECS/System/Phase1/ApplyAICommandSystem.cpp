#include "pch.h"
#include "ApplyAICommandSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const StaticSystemMetaStorage<3> ApplyAICommandSystem::kMetaStorage =
    MakeMetaStorage(
        SysTag<ApplyAICommandSystem>(),
        "ApplyAICommandSystem",
        std::array<AccessSpec, 3>
        {
            ReadSnapshot(ComponentRes<AIControlledTag>()),
            ReadSnapshot(ComponentRes<AICommandFrameComp>()),
            WriteImmediate(ComponentRes<ActorInputComp>()),
        });

void ApplyAICommandSystem::Execute(SystemContext& ctx)
{
	for (const auto& [entity, _, frame, input] :
		ctx.ecs.View<AIControlledTag, AICommandFrameComp, ActorInputComp>())
	{
		if (frame.hasMove)
		{
			input.move.inputX		= frame.moveDir.x;
			input.move.inputZ       = frame.moveDir.z;
			input.move.cameraYawRad = frame.moveYaw;
			input.move.wantsRun     = frame.wantsRun;
			input.move.lastUpdatedFrame = ctx.runtime.FrameIndex();
		}
		else
		{
			input.move.inputX = 0.0f;
			input.move.inputZ = 0.0f;
			input.move.cameraYawRad = 0.0f;
			input.move.wantsRun = false;
			input.move.lastUpdatedFrame = ctx.runtime.FrameIndex();
		}

		if (frame.hasAction)
		{
			input.action.directActionId = frame.actionId;
			input.action.directionX     = frame.actionDirX;
			input.action.directionZ     = frame.actionDirZ;
			input.action.requestedFrame = ctx.runtime.FrameIndex();
		}
	}
}

