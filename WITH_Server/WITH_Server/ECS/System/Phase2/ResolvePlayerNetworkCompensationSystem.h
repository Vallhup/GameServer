#pragma once

#include "System.h"
#include "SystemMetaStorage.h"
#include "../../GameplayRuntimeComponents.h"

class ResolvePlayerNetworkCompensationSystem final : public System
{
public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }

private:
	static const StaticSystemMetaStorage<2, 2, 2> kMetaStorage;

	static constexpr float kBaseAbilityInputBufferDurationSec = 0.18f;
	static constexpr float kMaxAbilityInputBufferDurationSec = 0.25f;
	static constexpr float kMaxJitterBudgetMs = 250.0f;
	static constexpr float kMinAttackDriftTolerance01 = 0.12f;
	static constexpr float kMaxAttackDriftTolerance01 = 0.25f;
	static constexpr float kReferenceAttackDurationMs = 1000.0f;
	static constexpr float kFallbackServerTickMs = 33.333f;
	static constexpr uint32_t kBaseInputDelayFrames = 1;
	static constexpr uint32_t kMinInputDelayFrames = 2;
	static constexpr uint32_t kMaxInputDelayFrames = 6;
	static constexpr uint32_t kMinInterpolationDelayFrames = 2;

	static float ComputeServerTickMs(double dtSec) noexcept;
	static float ComputeJitterBudgetMs(
		const PlayerNetworkTimingComp& timing) noexcept;
	static uint32_t ComputeInputDelayFrames(
		float jitterBudgetMs,
		float serverTickMs) noexcept;
	static uint32_t ComputeInterpolationDelayFrames(
		float jitterBudgetMs,
		float serverTickMs) noexcept;
	static float ComputeAbilityInputBufferDurationSec(
		float jitterBudgetMs) noexcept;
	static float ComputeAttackDriftTolerance01(
		const PlayerNetworkTimingComp& timing,
		float jitterBudgetMs) noexcept;
};
