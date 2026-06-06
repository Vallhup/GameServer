#include "pch.h"

#include <cassert>
#include <iostream>

#include "ECS/System/Phase0_AI/BossGimmickSystem.h"
#include "ECS/System/Phase8/BossGimmickCombatPolicy.h"
#include "RepComponent.h"
#include "WorldDef.h"
#include "WorldExecutionModelTypes.h"
#include "WorldRuntime.h"

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

	void RunFinalSafeZoneFailureQueuesDeathInterrupt()
	{
		WorldDef worldDef{};
		worldDef.id = WorldDefId::Final;
		WorldExecutionModel executionModel{};
		executionModel.key = 1;

		WorldRuntime runtime(WorldRuntimeCreateParams{
			.def = &worldDef,
			.executionModel = &executionModel
		});
		assert(runtime.Initialize());

		runtime.RegisterStorage<BossGimmickStateComp>();
		runtime.RegisterStorage<AIActionRuntimeComp>();
		runtime.RegisterStorage<AIMovementRuntimeComp>();
		runtime.RegisterStorage<AIIntentFrameComp>();
		runtime.RegisterStorage<PlayerControlIdentityComp>();
		runtime.RegisterStorage<WorldTransformComp>();
		runtime.RegisterStorage<CombatStatStateComp>();
		runtime.RegisterStorage<GimmickObjectComp>();
		runtime.RegisterStorage<SafeZoneComp>();
		runtime.RegisterStorage<BossGimmickImmunityComp>();
		runtime.RegisterStorage<StaticBoxHurtColliderComp>();
		runtime.RegisterStorage<PendingCombatResultComp>();
		runtime.RegisterStorage<DirtyFlagsComp>();
		runtime.RegisterStorage<PendingBossGimmickReplicationComp>();
		runtime.RegisterStorage<AbilityInterruptQueueComp>();
		runtime.RegisterStorage<PendingDespawnTag>();
		runtime.RegisterStorage<PendingWorldTransferTag>();
		runtime.FixStorages();

		const Entity boss = runtime.ReserveEntity();
		const Entity player = runtime.ReserveEntity();
		assert(!boss.IsNull() && !player.IsNull());

		BossGimmickStateComp gimmick{};
		gimmick.activeType = BossGimmickType::FinalSafeZone;
		gimmick.stage = BossGimmickStage::Resolve;
		gimmick.finalGimmickRequested = true;
		runtime.DeferredAddComponent<BossGimmickStateComp>(boss, gimmick);
		runtime.DeferredAddComponent<WorldTransformComp>(
			boss,
			WorldTransformComp{});

		CombatStatStateComp bossStats{};
		bossStats.currentHp = 1;
		bossStats.maxHp = 100;
		runtime.DeferredAddComponent<CombatStatStateComp>(boss, bossStats);
		runtime.DeferredAddComponent<AbilityInterruptQueueComp>(
			boss,
			AbilityInterruptQueueComp{});
		runtime.DeferredAddComponent<DirtyFlagsComp>(boss, DirtyFlagsComp{});
		runtime.DeferredAddComponent<PendingBossGimmickReplicationComp>(
			boss,
			PendingBossGimmickReplicationComp{});

		PlayerControlIdentityComp playerIdentity{};
		playerIdentity.ownerSessionId = 77;
		runtime.DeferredAddComponent<PlayerControlIdentityComp>(
			player,
			playerIdentity);
		runtime.DeferredAddComponent<WorldTransformComp>(
			player,
			WorldTransformComp{});

		CombatStatStateComp playerStats{};
		playerStats.currentHp = 80;
		playerStats.maxHp = 80;
		runtime.DeferredAddComponent<CombatStatStateComp>(player, playerStats);
		runtime.DeferredAddComponent<AbilityInterruptQueueComp>(
			player,
			AbilityInterruptQueueComp{});
		runtime.DeferredAddComponent<DirtyFlagsComp>(
			player,
			DirtyFlagsComp{});

		assert(runtime.BeginFrame(0, 0.0, 1.0 / 30.0));
		assert(runtime.FlushFrameCommands());
		assert(runtime.FlushLifecycleCommands());
		assert(runtime.BeginFrame(1, 1.0 / 30.0, 1.0 / 30.0));

		BossGimmickSystem system;
		SystemContext ctx{
			.runtime = runtime,
			.ecs = runtime.MakeView(),
			.dtSec = 1.0 / 30.0
		};
		system.Execute(ctx);

		const ECSView view = runtime.MakeView();
		const CombatStatStateComp* const resolvedPlayerStats =
			view.GetComponent<CombatStatStateComp>(player);
		const AbilityInterruptQueueComp* const playerInterrupts =
			view.GetComponent<AbilityInterruptQueueComp>(player);
		const DirtyFlagsComp* const playerDirty =
			view.GetComponent<DirtyFlagsComp>(player);
		const CombatStatStateComp* const resolvedBossStats =
			view.GetComponent<CombatStatStateComp>(boss);
		const AbilityInterruptQueueComp* const bossInterrupts =
			view.GetComponent<AbilityInterruptQueueComp>(boss);
		const DirtyFlagsComp* const bossDirty =
			view.GetComponent<DirtyFlagsComp>(boss);

		assert(resolvedPlayerStats != nullptr);
		assert(resolvedPlayerStats->currentHp == 0);
		assert(playerInterrupts != nullptr);
		assert(playerInterrupts->events.size() == 1);
		assert(playerInterrupts->events.front().cause ==
			AbilityTransitionCause::OnAttributeZero);
		assert(playerInterrupts->events.front().instigator == boss);
		assert(playerDirty != nullptr);
		assert(playerDirty->IsDirty(WorldDirtyType::Stat));

		assert(resolvedBossStats != nullptr);
		assert(resolvedBossStats->currentHp == 0);
		assert(bossInterrupts != nullptr);
		assert(bossInterrupts->events.size() == 1);
		assert(bossInterrupts->events.front().cause ==
			AbilityTransitionCause::OnAttributeZero);
		assert(bossInterrupts->events.front().instigator.IsNull());
		assert(bossDirty != nullptr);
		assert(bossDirty->IsDirty(WorldDirtyType::Stat));

		system.Execute(ctx);
		assert(playerInterrupts->events.size() == 1);
		assert(bossInterrupts->events.size() == 1);

		runtime.Shutdown();
	}
}

void RunBossGimmickCombatPolicySmokeTests()
{
	RunBossGimmickCombatPolicyBlocksActivePhaseTransition();
	RunBossGimmickCombatPolicyBlocksRequestedPhaseTransition();
	RunBossGimmickCombatPolicyBlocksRequestedFinalSafeZone();
	RunBossGimmickCombatPolicyAllowsCompletedGimmick();
	RunBossGimmickCombatPolicyAllowsIdleGimmick();
	RunFinalSafeZoneFailureQueuesDeathInterrupt();

	std::cout << "[PASS] BossGimmickCombatPolicy smoke\n";
}
