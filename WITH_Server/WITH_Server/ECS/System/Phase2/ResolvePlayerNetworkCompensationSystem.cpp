#include "pch.h"
#include "ResolvePlayerNetworkCompensationSystem.h"

#include "../GameplaySystemUtil.h"
#include "../Phase1/ApplyAICommandSystem.h"
#include "../Phase1/ApplyPlayerCommandSystem.h"
#include "ResolveAbilityStateSystem.h"
#include "ResolveLocomotionStateSystem.h"

#include <algorithm>
#include <cmath>

using namespace GameplaySystemUtil;

const StaticSystemMetaStorage<2, 2, 2>
ResolvePlayerNetworkCompensationSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<ResolvePlayerNetworkCompensationSystem>(),
		"ResolvePlayerNetworkCompensationSystem",
		std::array<AccessSpec, 2>
	{
		ReadImmediate(ComponentRes<PlayerNetworkTimingComp>()),
		WriteImmediate(ComponentRes<PlayerNetworkCompensationComp>()),
	},
		std::array<SystemTag, 2>
	{
		SysTag<ResolveAbilityStateSystem>(),
		SysTag<ResolveLocomotionStateSystem>(),
	},
		std::array<SystemTag, 2>
	{
		SysTag<ApplyAICommandSystem>(),
		SysTag<ApplyPlayerCommandSystem>(),
	});

void ResolvePlayerNetworkCompensationSystem::Execute(SystemContext& ctx)
{
	const float serverTickMs = ComputeServerTickMs(ctx.dtSec);

	for (auto [entity, timing, compensation] :
		ctx.ecs.View<
			PlayerNetworkTimingComp,
			PlayerNetworkCompensationComp>())
	{
		(void)entity;

		const float jitterBudgetMs = ComputeJitterBudgetMs(timing);
		compensation.jitterBudgetMs = jitterBudgetMs;
		compensation.abilityInputBufferDurationSec =
			ComputeAbilityInputBufferDurationSec(jitterBudgetMs);
		compensation.attackDriftTolerance01 =
			ComputeAttackDriftTolerance01(timing, jitterBudgetMs);
		compensation.inputDelayFrames =
			ComputeInputDelayFrames(jitterBudgetMs, serverTickMs);
		compensation.interpolationDelayFrames =
			ComputeInterpolationDelayFrames(jitterBudgetMs, serverTickMs);
		compensation.lastUpdatedFrame = ctx.runtime.FrameIndex();
		compensation.initialized = timing.initialized;
	}
}

float ResolvePlayerNetworkCompensationSystem::ComputeServerTickMs(
	double dtSec) noexcept
{
	if (dtSec <= 0.0)
	{
		return kFallbackServerTickMs;
	}

	return std::max(static_cast<float>(dtSec * 1000.0), 1.0f);
}

float ResolvePlayerNetworkCompensationSystem::ComputeJitterBudgetMs(
	const PlayerNetworkTimingComp& timing) noexcept
{
	if (!timing.initialized)
	{
		return 0.0f;
	}

	return ClampFloat(
		static_cast<float>(timing.rttVarMs),
		0.0f,
		kMaxRttVariationMs);
}

uint32_t ResolvePlayerNetworkCompensationSystem::ComputeInputDelayFrames(
	float jitterBudgetMs,
	float serverTickMs) noexcept
{
	const uint32_t jitterFrames =
		static_cast<uint32_t>(std::ceil(jitterBudgetMs / serverTickMs));
	return std::clamp(
		kBaseInputDelayFrames + jitterFrames,
		kMinInputDelayFrames,
		kMaxInputDelayFrames);
}

uint32_t ResolvePlayerNetworkCompensationSystem::ComputeInterpolationDelayFrames(
	float jitterBudgetMs,
	float serverTickMs) noexcept
{
	const uint32_t jitterFrames =
		static_cast<uint32_t>(std::ceil(jitterBudgetMs / serverTickMs));
	return std::max(
		kMinInterpolationDelayFrames,
		jitterFrames + 1);
}

float ResolvePlayerNetworkCompensationSystem::ComputeAbilityInputBufferDurationSec(
	float jitterBudgetMs) noexcept
{
	return ClampFloat(
		kBaseAbilityInputBufferDurationSec + (jitterBudgetMs * 0.001f),
		kBaseAbilityInputBufferDurationSec,
		kMaxAbilityInputBufferDurationSec);
}

float ResolvePlayerNetworkCompensationSystem::ComputeAttackDriftTolerance01(
	const PlayerNetworkTimingComp& timing,
	float jitterBudgetMs) noexcept
{
	if (!timing.initialized)
	{
		return kMaxAttackDriftTolerance01;
	}

	return ClampFloat(
		kMinAttackDriftTolerance01 +
			(jitterBudgetMs / kReferenceAttackDurationMs),
		kMinAttackDriftTolerance01,
		kMaxAttackDriftTolerance01);
}
