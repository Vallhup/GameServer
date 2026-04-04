#include "pch.h"
#include "ComputeLocomotionMoveDeltaSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta ComputeLocomotionMoveDeltaSystem::kMeta =
	MakeSystemMeta<ComputeLocomotionMoveDeltaSystem>(
		"ComputeLocomotionMoveDeltaSystem");

void ComputeLocomotionMoveDeltaSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, locomotionState, actionState, moveDelta] :
		ctx.ecs.View<
			LocomotionStateComp,
			ActionStateComp,
			LocomotionMoveDeltaComp>())
	{
		(void)entity;
		moveDelta = {};

		if (IsActionActive(actionState) ||
			locomotionState.mode == LocomotionMode::Idle)
		{
			continue;
		}

		moveDelta.deltaPosition.x = locomotionState.desiredMoveDirX *
			locomotionState.currentSpeed * static_cast<float>(ctx.dtSec);
		moveDelta.deltaPosition.z = locomotionState.desiredMoveDirZ *
			locomotionState.currentSpeed * static_cast<float>(ctx.dtSec);
		moveDelta.hasDelta = true;
	}
}

const SystemMeta& ComputeLocomotionMoveDeltaSystem::Meta() const
{
	return kMeta;
}
