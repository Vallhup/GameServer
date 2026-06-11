#include "pch.h"
#include "AIMovementDistanceResolver.h"

AIMovementDistanceBand AIMovementDistanceResolver::ResolveBand(
	const AIMovementProfileDef& profile,
	double distance,
	AIMovementRuntimeComp* runtime) noexcept
{
	AIMovementDistanceBand band = ClassifyBand(profile, distance);

	if (runtime != nullptr &&
		runtime->hasCombatDistanceBand &&
		CanRetainBand(
			profile,
			runtime->combatDistanceBand,
			distance))
	{
		band = runtime->combatDistanceBand;
	}

	if (runtime != nullptr)
	{
		runtime->combatDistanceBand = band;
		runtime->hasCombatDistanceBand = true;
	}

	return band;
}

AIMovementBehavior AIMovementDistanceResolver::ResolveBehavior(
	const AIMovementProfileDef& profile,
	double distance,
	AIMovementRuntimeComp* runtime) noexcept
{
	return ResolveBehaviorForBand(
		profile,
		ResolveBand(profile, distance, runtime));
}

AIMovementDistanceBand AIMovementDistanceResolver::ClassifyBand(
	const AIMovementProfileDef& profile,
	double distance) noexcept
{
	if (distance <= profile.veryCloseDistance)
		return AIMovementDistanceBand::VeryClose;

	if (profile.preferredMinDistance <= profile.preferredMaxDistance &&
		distance >= profile.preferredMinDistance &&
		distance <= profile.preferredMaxDistance)
	{
		return AIMovementDistanceBand::Preferred;
	}

	if (distance <= profile.closeDistance)
		return AIMovementDistanceBand::Close;

	if (distance <= profile.midDistance)
		return AIMovementDistanceBand::Mid;

	return AIMovementDistanceBand::Far;
}

bool AIMovementDistanceResolver::CanRetainBand(
	const AIMovementProfileDef& profile,
	AIMovementDistanceBand band,
	double distance) noexcept
{
	const double hysteresis = std::max(0.0, profile.distanceHysteresis);
	if (hysteresis <= 0.0)
		return false;

	const double lowerSample = std::max(0.0, distance - hysteresis);
	const double upperSample = distance + hysteresis;

	return
		ClassifyBand(profile, distance) == band ||
		ClassifyBand(profile, lowerSample) == band ||
		ClassifyBand(profile, upperSample) == band;
}

AIMovementBehavior AIMovementDistanceResolver::ResolveBehaviorForBand(
	const AIMovementProfileDef& profile,
	AIMovementDistanceBand band) noexcept
{
	switch (band) {
	case AIMovementDistanceBand::VeryClose:
		return profile.veryCloseBehavior;
	case AIMovementDistanceBand::Close:
		return profile.closeBehavior;
	case AIMovementDistanceBand::Preferred:
		return profile.preferredBehavior;
	case AIMovementDistanceBand::Mid:
		return profile.midBehavior;
	case AIMovementDistanceBand::Far:
	default:
		return profile.farBehavior;
	}
}
