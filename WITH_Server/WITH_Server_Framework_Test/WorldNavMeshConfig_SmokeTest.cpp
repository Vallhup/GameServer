#include "pch.h"

#include <cassert>
#include <cmath>
#include <iostream>

#include "WorldDef.h"

namespace
{
	constexpr float kExpectedAgentRadius = 0.34f;
	constexpr float kExpectedAgentHeight = 1.8f;
	constexpr float kExpectedAgentMaxClimb = 0.3f;
	constexpr float kExpectedAgentMaxSlope = 40.5f;

	bool AlmostEqual(float lhs, float rhs)
	{
		return std::fabs(lhs - rhs) < 0.0001f;
	}

	void AssertNavMeshMatchesUnityProfile(const WorldDef& worldDef)
	{
		assert(worldDef.map.navMesh.has_value());

		const MapNavMeshDef& navMesh = worldDef.map.navMesh.value();
		assert(AlmostEqual(navMesh.agentRadius, kExpectedAgentRadius));
		assert(AlmostEqual(navMesh.agentHeight, kExpectedAgentHeight));
		assert(AlmostEqual(navMesh.agentMaxClimb, kExpectedAgentMaxClimb));
		assert(AlmostEqual(navMesh.agentMaxSlope, kExpectedAgentMaxSlope));
	}
}

void RunWorldNavMeshConfigSmokeTests()
{
	AssertNavMeshMatchesUnityProfile(CreatePlazaWorldDef(1));
	AssertNavMeshMatchesUnityProfile(CreateVillageWorldDef(1));
	AssertNavMeshMatchesUnityProfile(CreateCastleWorldDef(1));
	AssertNavMeshMatchesUnityProfile(CreateFinalWorldDef(1));

	std::cout << "[PASS] World NavMesh config matches Unity profile.\n";
}
