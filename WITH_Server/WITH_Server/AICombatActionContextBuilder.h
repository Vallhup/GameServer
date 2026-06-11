#pragma once

#include "AICombatActionSelectionTypes.h"

class AICombatActionContextBuilder final {
public:
	static bool CanBuild(const AIContext& ctx) noexcept;
	static AICombatActionSelectionContext Build(
		const AIContext& ctx) noexcept;

private:
	static uint8_t ResolvePhase(const AIContext& ctx) noexcept;

	static const AIMovementProfileDef* SelectMovementProfile(
		const AIContext& ctx,
		uint8_t phase) noexcept;

	static AIActionDistanceBucket ResolveDistanceBucket(
		const AIMovementProfileDef* movement,
		double distance) noexcept;

	static float ResolveHpRatio(
		const CombatStatStateComp* stats) noexcept;

	static float ResolveTargetHpRatio(const AIContext& ctx) noexcept;
};
