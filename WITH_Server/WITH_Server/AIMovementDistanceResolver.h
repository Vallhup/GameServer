#pragma once

#include "AIBehaviorDef.h"
#include "ECS/Components/GameplayAIComponents.h"

class AIMovementDistanceResolver final {
public:
	static AIMovementDistanceBand ResolveBand(
		const AIMovementProfileDef& profile,
		double distance,
		AIMovementRuntimeComp* runtime) noexcept;

	static AIMovementBehavior ResolveBehavior(
		const AIMovementProfileDef& profile,
		double distance,
		AIMovementRuntimeComp* runtime) noexcept;

private:
	static AIMovementDistanceBand ClassifyBand(
		const AIMovementProfileDef& profile,
		double distance) noexcept;

	static bool CanRetainBand(
		const AIMovementProfileDef& profile,
		AIMovementDistanceBand band,
		double distance) noexcept;

	static AIMovementBehavior ResolveBehaviorForBand(
		const AIMovementProfileDef& profile,
		AIMovementDistanceBand band) noexcept;
};
