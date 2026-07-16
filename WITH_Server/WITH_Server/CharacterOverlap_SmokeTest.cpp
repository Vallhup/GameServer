#include "pch.h"

#include <cassert>
#include <cmath>
#include <iostream>

#include "ECS/GameplayRuntimeComponents.h"
#include "ECS/System/Phase5/ResolveCharacterOverlapSystem.h"
#include "WorldDef.h"
#include "WorldExecutionModelTypes.h"
#include "WorldRuntime.h"

namespace
{
	struct OverlapResult
	{
		float lhsX{ 0.0f };
		float rhsX{ 0.0f };
		bool lhsAdjusted{ false };
		bool rhsAdjusted{ false };
	};

	bool NearlyEqual(float lhs, float rhs)
	{
		return std::abs(lhs - rhs) < 0.0001f;
	}

	OverlapResult ResolvePair(
		BodyPushability lhsPushability,
		BodyPushability rhsPushability)
	{
		WorldDef worldDef{};
		worldDef.id = WorldDefId::Village;
		WorldExecutionModel executionModel{};
		executionModel.key = 1;

		WorldRuntime runtime(WorldRuntimeCreateParams{
			.def = &worldDef,
			.executionModel = &executionModel
			});
		assert(runtime.Initialize());
		runtime.RegisterStorage<WorldTransformComp>();
		runtime.RegisterStorage<PreCollisionTransformComp>();
		runtime.RegisterStorage<BodyCollisionShapeComp>();
		runtime.RegisterStorage<BodyCollisionResolveComp>();
		runtime.RegisterStorage<DirtyFlagsComp>();
		runtime.RegisterStorage<PendingDespawnTag>();
		runtime.RegisterStorage<PendingWorldTransferTag>();
		runtime.FixStorages();

		const Entity lhs = runtime.ReserveEntity();
		const Entity rhs = runtime.ReserveEntity();
		assert(!lhs.IsNull());
		assert(!rhs.IsNull());

		WorldTransformComp lhsTransform{};
		lhsTransform.position = { 0.0f, 0.0f, 0.0f };
		WorldTransformComp rhsTransform{};
		rhsTransform.position = { 0.5f, 0.0f, 0.0f };

		BodyCollisionShapeComp lhsShape{};
		lhsShape.pushability = lhsPushability;
		lhsShape.maxOverlapCorrectionPerFrameXZ = 1.0f;
		BodyCollisionShapeComp rhsShape{};
		rhsShape.pushability = rhsPushability;
		rhsShape.maxOverlapCorrectionPerFrameXZ = 1.0f;

		runtime.DeferredUpsertComponent(lhs, lhsTransform);
		runtime.DeferredAddComponent<PreCollisionTransformComp>(lhs);
		runtime.DeferredUpsertComponent(lhs, lhsShape);
		runtime.DeferredAddComponent<BodyCollisionResolveComp>(lhs);
		runtime.DeferredAddComponent<DirtyFlagsComp>(lhs);

		runtime.DeferredUpsertComponent(rhs, rhsTransform);
		runtime.DeferredAddComponent<PreCollisionTransformComp>(rhs);
		runtime.DeferredUpsertComponent(rhs, rhsShape);
		runtime.DeferredAddComponent<BodyCollisionResolveComp>(rhs);
		runtime.DeferredAddComponent<DirtyFlagsComp>(rhs);

		assert(runtime.BeginFrame(0, 0.0, 1.0 / 30.0));
		assert(runtime.FlushFrameCommands());
		assert(runtime.FlushLifecycleCommands());
		assert(runtime.BeginFrame(1, 1.0 / 30.0, 1.0 / 30.0));

		ResolveCharacterOverlapSystem system;
		SystemContext ctx{
			.runtime = runtime,
			.ecs = runtime.MakeView(),
			.dtSec = 1.0 / 30.0
		};
		system.Execute(ctx);

		const ECSView view = runtime.MakeView();
		const WorldTransformComp* const resolvedLhsTransform =
			view.GetComponent<WorldTransformComp>(lhs);
		const WorldTransformComp* const resolvedRhsTransform =
			view.GetComponent<WorldTransformComp>(rhs);
		const BodyCollisionResolveComp* const lhsResolve =
			view.GetComponent<BodyCollisionResolveComp>(lhs);
		const BodyCollisionResolveComp* const rhsResolve =
			view.GetComponent<BodyCollisionResolveComp>(rhs);
		assert(resolvedLhsTransform != nullptr);
		assert(resolvedRhsTransform != nullptr);
		assert(lhsResolve != nullptr);
		assert(rhsResolve != nullptr);

		return OverlapResult{
			.lhsX = resolvedLhsTransform->position.x,
			.rhsX = resolvedRhsTransform->position.x,
			.lhsAdjusted = lhsResolve->overlapAdjusted,
			.rhsAdjusted = rhsResolve->overlapAdjusted
		};
	}
}

void RunCharacterOverlapSmokeTests()
{
	const OverlapResult kinematicPair = ResolvePair(
		BodyPushability::Kinematic,
		BodyPushability::Kinematic);
	assert(NearlyEqual(kinematicPair.lhsX, -0.25f));
	assert(NearlyEqual(kinematicPair.rhsX, 0.75f));
	assert(kinematicPair.lhsAdjusted);
	assert(kinematicPair.rhsAdjusted);

	const OverlapResult dynamicKinematicPair = ResolvePair(
		BodyPushability::Dynamic,
		BodyPushability::Kinematic);
	assert(NearlyEqual(dynamicKinematicPair.lhsX, -0.5f));
	assert(NearlyEqual(dynamicKinematicPair.rhsX, 0.5f));
	assert(dynamicKinematicPair.lhsAdjusted);
	assert(!dynamicKinematicPair.rhsAdjusted);

	std::cout << "[PASS] CharacterOverlap smoke\n";
}
