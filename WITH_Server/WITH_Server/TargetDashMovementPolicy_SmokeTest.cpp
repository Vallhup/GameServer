#include "ECS/System/Phase4/TargetDashMovementPolicy.h"
#include "AbilityDef.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
	void ExpectNear(float actual, float expected, const char* message)
	{
		if (std::fabs(actual - expected) > 0.0001f)
		{
			throw std::runtime_error(message);
		}
	}
}

void RunTargetDashMovementPolicySmokeTests()
{
	AbilityMovementSegmentDef movementSegment{};
	if (movementSegment.targetStopDistanceOffset.has_value())
	{
		throw std::runtime_error(
			"target stop distance offset must default to unset");
	}
	movementSegment.targetStopDistanceOffset = 0.35f;
	ExpectNear(
		*movementSegment.targetStopDistanceOffset,
		0.35f,
		"ability movement segment did not retain target stop distance offset");

	ExpectNear(
		TargetDashMovementPolicy::ResolveStopDistance(1.5f, 0.42f, 0.0f),
		1.97f,
		"default target dash stop distance changed");
	ExpectNear(
		TargetDashMovementPolicy::ResolveStopDistance(1.5f, 0.42f, 0.35f),
		2.32f,
		"target dash stop distance offset was not applied");
}

#ifdef TARGET_DASH_POLICY_STANDALONE
int main()
{
	RunTargetDashMovementPolicySmokeTests();
	std::cout << "Target dash movement policy smoke tests passed.\n";
	return 0;
}
#endif
