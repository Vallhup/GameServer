#include "pch.h"

#include <cassert>
#include <iostream>

#include "ECS/System/Phase8/BossGimmickCombatPolicy.h"

namespace
{
	void RunBossGimmickCombatPolicyBlocksActivePhaseTransition()
	{
		BossGimmickStateComp gimmick{};
		gimmick.activeType = BossGimmickType::PhaseTransitionObjects;
		gimmick.stage = BossGimmickStage::Active;

		assert(BossGimmickCombatPolicy::ShouldIgnoreIncomingHpDamage(gimmick));
	}

	void RunBossGimmickCombatPolicyBlocksRequestedPhaseTransition()
	{
		BossGimmickStateComp gimmick{};
		gimmick.phaseTransitionGimmickRequested = true;

		assert(BossGimmickCombatPolicy::ShouldIgnoreIncomingHpDamage(gimmick));
	}

	void RunBossGimmickCombatPolicyBlocksRequestedFinalSafeZone()
	{
		BossGimmickStateComp gimmick{};
		gimmick.finalGimmickRequested = true;

		assert(BossGimmickCombatPolicy::ShouldIgnoreIncomingHpDamage(gimmick));
	}

	void RunBossGimmickCombatPolicyAllowsCompletedGimmick()
	{
		BossGimmickStateComp gimmick{};
		gimmick.activeType = BossGimmickType::FinalSafeZone;
		gimmick.stage = BossGimmickStage::Completed;
		gimmick.finalGimmickCompleted = true;

		assert(!BossGimmickCombatPolicy::ShouldIgnoreIncomingHpDamage(gimmick));
	}

	void RunBossGimmickCombatPolicyAllowsIdleGimmick()
	{
		BossGimmickStateComp gimmick{};

		assert(!BossGimmickCombatPolicy::ShouldIgnoreIncomingHpDamage(gimmick));
	}
}

void RunBossGimmickCombatPolicySmokeTests()
{
	RunBossGimmickCombatPolicyBlocksActivePhaseTransition();
	RunBossGimmickCombatPolicyBlocksRequestedPhaseTransition();
	RunBossGimmickCombatPolicyBlocksRequestedFinalSafeZone();
	RunBossGimmickCombatPolicyAllowsCompletedGimmick();
	RunBossGimmickCombatPolicyAllowsIdleGimmick();

	std::cout << "[PASS] BossGimmickCombatPolicy smoke\n";
}
