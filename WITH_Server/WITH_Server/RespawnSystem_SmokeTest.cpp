#include "pch.h"

#include <cassert>
#include <cmath>
#include <iostream>

#include "ECS/GameplayRuntimeComponents.h"
#include "ECS/System/Phase8/PlayerDeathStatePolicy.h"
#include "ECS/System/Phase8/ResolveDeathAndDespawnSystem.h"
#include "GameDataCatalog.h"
#include "GameplayContentCatalog.h"
#include "ServerPathResolver.h"
#include "WorldDef.h"
#include "WorldExecutionModelTypes.h"
#include "WorldRuntime.h"

namespace
{
	const AbilityDef& FindKnightDeadAbility(
		const GameplayContentCatalogSnapshot& catalog)
	{
		for (const AbilityDef& ability : catalog.Abilities().GetAll())
		{
			if (ability.key == "Ability.Knight_Dead")
			{
				return ability;
			}
		}

		assert(false && "Knight dead ability must exist");
		return catalog.Abilities().GetAll().front();
	}

	void RegisterRespawnTestStorages(WorldRuntime& runtime)
	{
		runtime.RegisterStorage<CombatStatStateComp>();
		runtime.RegisterStorage<AbilityStateComp>();
		runtime.RegisterStorage<AbilityInterruptQueueComp>();
		runtime.RegisterStorage<PlayerControlIdentityComp>();
		runtime.RegisterStorage<PlayerDeathCountConsumedTag>();
		runtime.RegisterStorage<PendingDespawnTag>();
		runtime.RegisterStorage<PendingPlayerDeathCountEventComp>();
		runtime.RegisterStorage<PlayerDeathStateComp>();
		runtime.RegisterStorage<WorldTransformComp>();
		runtime.RegisterStorage<AnimationPlaybackStateComp>();
		runtime.RegisterStorage<LocomotionMoveDeltaComp>();
		runtime.RegisterStorage<AbilityMoveDeltaComp>();
		runtime.RegisterStorage<PreCollisionTransformComp>();
		runtime.RegisterStorage<DirtyFlagsComp>();
		runtime.RegisterStorage<ConsumableInventoryComp>();
		runtime.RegisterStorage<ActorInputComp>();
		runtime.RegisterStorage<LocomotionStateComp>();
		runtime.RegisterStorage<AbilityTimelineAdvanceComp>();
		runtime.RegisterStorage<StaminaRecoveryStateComp>();
		runtime.RegisterStorage<SpawnTypeComp>();
	}

	void QueueDeadPlayer(
		WorldRuntime& runtime,
		Entity entity,
		const AbilityDef& deadAbility,
		PlayerDeathState initialDeathState =
			PlayerDeathState::AwaitingRespawnInput,
		bool respawnRequested = true,
		bool deathCountConsumed = true)
	{
		CombatStatStateComp stats{};
		stats.currentHp = 0;
		stats.maxHp = 120;
		stats.currentStamina = 3;
		stats.maxStamina = 70;
		stats.currentPoise = 4;
		stats.maxPoise = 30;
		runtime.DeferredUpsertComponent(entity, stats);

		AbilityStateComp ability{};
		ability.abilityId = deadAbility.id;
		ability.abilityInstanceId = 41;
		ability.elapsedSec = deadAbility.timeline.durationSec;
		runtime.DeferredUpsertComponent(entity, ability);

		AbilityInterruptQueueComp interruptQueue{};
		interruptQueue.events.push_back(AbilityInterruptEvent{
			.cause = AbilityTransitionCause::OnAttributeZero,
			.frameIndex = 1,
			.priority = 1000
		});
		runtime.DeferredUpsertComponent(entity, interruptQueue);

		PlayerControlIdentityComp player{};
		player.ownerSessionId = 77;
		runtime.DeferredUpsertComponent(entity, player);

		PlayerDeathStateComp deathState{};
		deathState.state = initialDeathState;
		deathState.respawnRequested = respawnRequested;
		deathState.deathCountRevision = 9;
		runtime.DeferredUpsertComponent(entity, deathState);

		PendingPlayerDeathCountEventComp deathEvent{};
		deathEvent.pending = false;
		deathEvent.killerCharacterId = CharacterId::Imp;
		runtime.DeferredUpsertComponent(entity, deathEvent);

		WorldTransformComp transform{};
		transform.position = { 11.0f, 22.0f, 33.0f };
		transform.rotation = { 0.0f, 0.5f, 0.0f, 0.8660254f };
		runtime.DeferredUpsertComponent(entity, transform);

		AnimationPlaybackStateComp playback{};
		playback.source = AnimationPlaybackSource::Ability;
		playback.animationId = AnimationId::Knight_Death;
		playback.boundAbilityId = deadAbility.id;
		playback.boundAbilityInstanceId = ability.abilityInstanceId;
		playback.normalizedTime = 1.0f;
		playback.holdLastFrame = true;
		runtime.DeferredUpsertComponent(entity, playback);

		ActorInputComp input{};
		input.move.inputX = 1.0f;
		input.move.inputZ = -1.0f;
		input.move.wantsRun = true;
		input.guard.isPressed = true;
		input.ability.type = PlayerAbilityInputType::LightAttack;
		input.abilityBuffer.hasEvent = true;
		runtime.DeferredUpsertComponent(entity, input);

		LocomotionStateComp locomotion{};
		locomotion.mode = LocomotionMode::Run;
		locomotion.facingYawRad = 1.25f;
		locomotion.currentSpeed = 4.0f;
		locomotion.wasLocomotionMoving = true;
		runtime.DeferredUpsertComponent(entity, locomotion);

		AbilityTimelineAdvanceComp advance{};
		advance.abilityId = deadAbility.id;
		advance.abilityInstanceId = ability.abilityInstanceId;
		advance.currElapsedSec = deadAbility.timeline.durationSec;
		runtime.DeferredUpsertComponent(entity, advance);

		StaminaRecoveryStateComp recovery{};
		recovery.regenLockRemainingSec = 5.0f;
		recovery.regenRemainder = 0.75f;
		runtime.DeferredUpsertComponent(entity, recovery);

		ConsumableInventoryComp inventory{};
		inventory.hpPotionCount = 0;
		runtime.DeferredUpsertComponent(entity, inventory);

		SpawnTypeComp spawnType{};
		spawnType.characterId = CharacterId::Knight;
		runtime.DeferredUpsertComponent(entity, spawnType);

		runtime.DeferredAddComponent<LocomotionMoveDeltaComp>(entity);
		runtime.DeferredAddComponent<AbilityMoveDeltaComp>(entity);
		runtime.DeferredAddComponent<PreCollisionTransformComp>(entity);
		runtime.DeferredAddComponent<DirtyFlagsComp>(entity);
		if (deathCountConsumed)
		{
			runtime.DeferredAddComponent<PlayerDeathCountConsumedTag>(entity);
		}
	}

	bool NearlyEqual(float lhs, float rhs)
	{
		return std::abs(lhs - rhs) < 0.0001f;
	}
}

void RunRespawnSystemSmokeTests()
{
	{
		PlayerDeathStateComp deathState{};
		assert(!PlayerDeathStatePolicy::CanLatchRespawnRequest(deathState));

		deathState.state = PlayerDeathState::WaitingForDeathCount;
		deathState.respawnRequested = true;
		assert(!PlayerDeathStatePolicy::CanLatchRespawnRequest(deathState));

		assert(PlayerDeathStatePolicy::ApplyDeathCountDecision(
			deathState,
			true,
			17));
		assert(deathState.state == PlayerDeathState::AwaitingRespawnInput);
		assert(deathState.deathCountRevision == 17);
		assert(!deathState.respawnRequested);
		assert(PlayerDeathStatePolicy::CanLatchRespawnRequest(deathState));

		deathState.state = PlayerDeathState::WaitingForDeathCount;
		deathState.respawnRequested = true;
		assert(PlayerDeathStatePolicy::ApplyDeathCountDecision(
			deathState,
			false,
			18));
		assert(deathState.state == PlayerDeathState::DeathCountExhausted);
		assert(!deathState.respawnRequested);

		assert(!PlayerDeathStatePolicy::ShouldReturnExhaustedPartyToPlaza(
			false,
			3,
			0));
		assert(!PlayerDeathStatePolicy::ShouldReturnExhaustedPartyToPlaza(
			true,
			0,
			0));
		assert(!PlayerDeathStatePolicy::ShouldReturnExhaustedPartyToPlaza(
			true,
			3,
			1));
		assert(PlayerDeathStatePolicy::ShouldReturnExhaustedPartyToPlaza(
			true,
			3,
			0));
	}

	GameplayContentCatalogSnapshot catalog;
	const DefLoadResult loadResult = catalog.LoadFromRoots(
		GameplayContentCatalogRoots{
			.attributeRoot = ServerPathResolver::GetDefaultAttributeDefRoot(),
			.tagRoot = ServerPathResolver::GetDefaultGameplayTagDefRoot(),
			.effectRoot = ServerPathResolver::GetDefaultGameplayEffectDefRoot(),
			.projectileRoot = ServerPathResolver::GetDefaultProjectileDefRoot(),
			.areaHitRoot = ServerPathResolver::GetDefaultAreaHitDefRoot(),
			.abilityRoot = ServerPathResolver::GetDefaultAbilityDefRoot(),
			.abilitySetRoot = ServerPathResolver::GetDefaultAbilitySetDefRoot()
		});
	assert(loadResult.succeeded);
	GameplayContentCatalogSnapshot::Publish(catalog);

	GameDataCatalog gameDataCatalog;
	const DefLoadResult gameDataLoadResult = gameDataCatalog.LoadFromRoots(
		GameDataCatalogRoots{
			.characterRoot = ServerPathResolver::GetDefaultCharacterDefRoot(),
			.aiBehaviorRoot = ServerPathResolver::GetDefaultAIBehaviorDefRoot(),
			.spawnSetRoot = ServerPathResolver::GetDefaultSpawnSetDefRoot(),
			.titleRoot = ServerPathResolver::GetDefaultTitleDefRoot()
		});
	assert(gameDataLoadResult.succeeded);
	GameDataCatalog::Publish(gameDataCatalog);

	const AbilityDef& deadAbility = FindKnightDeadAbility(catalog);
	WorldDef worldDef{};
	worldDef.id = WorldDefId::Village;
	WorldExecutionModel executionModel{};
	executionModel.key = 1;

	WorldRuntime runtime(WorldRuntimeCreateParams{
		.def = &worldDef,
		.executionModel = &executionModel
		});
	assert(runtime.Initialize());
	RegisterRespawnTestStorages(runtime);
	runtime.FixStorages();

	const Entity player = runtime.ReserveEntity();
	assert(!player.IsNull());
	QueueDeadPlayer(runtime, player, deadAbility);

	const Entity earlyRequestPlayer = runtime.ReserveEntity();
	assert(!earlyRequestPlayer.IsNull());
	QueueDeadPlayer(
		runtime,
		earlyRequestPlayer,
		deadAbility,
		PlayerDeathState::Alive,
		true,
		false);

	assert(runtime.BeginFrame(0, 0.0, 1.0 / 30.0));
	assert(runtime.FlushFrameCommands());
	assert(runtime.FlushLifecycleCommands());
	assert(runtime.BeginFrame(1, 1.0 / 30.0, 1.0 / 30.0));

	ResolveDeathAndDespawnSystem system;
	SystemContext ctx{
		.runtime = runtime,
		.ecs = runtime.MakeView(),
		.dtSec = 1.0 / 30.0
	};
	system.Execute(ctx);

	ECSView view = runtime.MakeView();
	PlayerDeathStateComp* earlyRequestDeathState =
		view.GetMutableComponent<PlayerDeathStateComp>(earlyRequestPlayer);
	const CombatStatStateComp* earlyRequestStats =
		view.GetComponent<CombatStatStateComp>(earlyRequestPlayer);
	assert(earlyRequestDeathState != nullptr);
	assert(earlyRequestDeathState->state ==
		PlayerDeathState::WaitingForDeathCount);
	assert(!earlyRequestDeathState->respawnRequested);
	assert(earlyRequestStats != nullptr && earlyRequestStats->currentHp == 0);
	assert(PlayerDeathStatePolicy::ApplyDeathCountDecision(
		*earlyRequestDeathState,
		true,
		23));

	const CombatStatStateComp* const stats =
		view.GetComponent<CombatStatStateComp>(player);
	const ConsumableInventoryComp* const inventory =
		view.GetComponent<ConsumableInventoryComp>(player);
	const AbilityStateComp* const ability =
		view.GetComponent<AbilityStateComp>(player);
	const AbilityInterruptQueueComp* const interruptQueue =
		view.GetComponent<AbilityInterruptQueueComp>(player);
	const ActorInputComp* const input =
		view.GetComponent<ActorInputComp>(player);
	const LocomotionStateComp* const locomotion =
		view.GetComponent<LocomotionStateComp>(player);
	const WorldTransformComp* const transform =
		view.GetComponent<WorldTransformComp>(player);
	const PreCollisionTransformComp* const preCollision =
		view.GetComponent<PreCollisionTransformComp>(player);
	const AnimationPlaybackStateComp* const playback =
		view.GetComponent<AnimationPlaybackStateComp>(player);
	const PlayerDeathStateComp* const deathState =
		view.GetComponent<PlayerDeathStateComp>(player);
	const DirtyFlagsComp* const dirty =
		view.GetComponent<DirtyFlagsComp>(player);

	assert(stats != nullptr);
	assert(stats->currentHp == stats->maxHp);
	assert(stats->currentStamina == stats->maxStamina);
	assert(stats->currentPoise == stats->maxPoise);
	assert(inventory != nullptr && inventory->hpPotionCount == 3);
	assert(ability != nullptr);
	assert(ability->abilityId == InvalidAbilityId);
	assert(ability->abilityInstanceId == 42);
	assert(interruptQueue != nullptr);
	assert(interruptQueue->events.empty());
	assert(input != nullptr);
	assert(input->move.inputX == 0.0f && input->move.inputZ == 0.0f);
	assert(!input->move.wantsRun);
	assert(!input->guard.isPressed);
	assert(input->ability.type == PlayerAbilityInputType::None);
	assert(!input->abilityBuffer.hasEvent);
	assert(locomotion != nullptr);
	assert(locomotion->mode == LocomotionMode::Idle);
	assert(NearlyEqual(locomotion->facingYawRad, 1.25f));
	assert(transform != nullptr);
	assert(NearlyEqual(transform->position.x, 11.0f));
	assert(NearlyEqual(transform->position.y, 22.0f));
	assert(NearlyEqual(transform->position.z, 33.0f));
	assert(preCollision != nullptr);
	assert(NearlyEqual(preCollision->prevPosition.x, transform->position.x));
	assert(NearlyEqual(preCollision->prevPosition.y, transform->position.y));
	assert(NearlyEqual(preCollision->prevPosition.z, transform->position.z));
	assert(playback != nullptr);
	assert(playback->source == AnimationPlaybackSource::Locomotion);
	assert(playback->animationId == AnimationId::Knight_Idle);
	assert(playback->boundLocomotionMode == LocomotionMode::Idle);
	assert(deathState != nullptr);
	assert(deathState->state == PlayerDeathState::Alive);
	assert(!deathState->respawnRequested);
	assert(dirty != nullptr);
	assert(dirty->IsDirty(WorldDirtyType::Stat));
	assert(dirty->IsDirty(WorldDirtyType::Transform));
	assert(dirty->IsDirty(WorldDirtyType::Animation));
	assert(dirty->IsDirty(WorldDirtyType::Inventory));

	assert(runtime.FlushFrameCommands());
	assert(!view.HasComponent<PlayerDeathCountConsumedTag>(player));
	assert(runtime.FlushLifecycleCommands());
	assert(runtime.BeginFrame(2, 2.0 / 30.0, 1.0 / 30.0));
	system.Execute(ctx);

	earlyRequestDeathState =
		view.GetMutableComponent<PlayerDeathStateComp>(earlyRequestPlayer);
	earlyRequestStats =
		view.GetComponent<CombatStatStateComp>(earlyRequestPlayer);
	assert(earlyRequestDeathState != nullptr);
	assert(earlyRequestDeathState->state ==
		PlayerDeathState::AwaitingRespawnInput);
	assert(!earlyRequestDeathState->respawnRequested);
	assert(earlyRequestStats != nullptr);
	assert(earlyRequestStats->currentHp == 0);
	assert(runtime.FlushFrameCommands());
	assert(view.HasComponent<PlayerDeathCountConsumedTag>(earlyRequestPlayer));
	assert(runtime.FlushLifecycleCommands());
	runtime.Shutdown();

	WorldDef plazaWorldDef{};
	plazaWorldDef.id = WorldDefId::Plaza;
	WorldRuntime plazaRuntime(WorldRuntimeCreateParams{
		.def = &plazaWorldDef,
		.executionModel = &executionModel
		});
	assert(plazaRuntime.Initialize());
	RegisterRespawnTestStorages(plazaRuntime);
	plazaRuntime.FixStorages();

	const Entity plazaPlayer = plazaRuntime.ReserveEntity();
	assert(!plazaPlayer.IsNull());
	QueueDeadPlayer(plazaRuntime, plazaPlayer, deadAbility);
	assert(plazaRuntime.BeginFrame(0, 0.0, 1.0 / 30.0));
	assert(plazaRuntime.FlushFrameCommands());
	assert(plazaRuntime.FlushLifecycleCommands());
	assert(plazaRuntime.BeginFrame(1, 1.0 / 30.0, 1.0 / 30.0));

	SystemContext plazaCtx{
		.runtime = plazaRuntime,
		.ecs = plazaRuntime.MakeView(),
		.dtSec = 1.0 / 30.0
	};
	system.Execute(plazaCtx);

	const ECSView plazaView = plazaRuntime.MakeView();
	const CombatStatStateComp* const plazaStats =
		plazaView.GetComponent<CombatStatStateComp>(plazaPlayer);
	const PlayerDeathStateComp* const plazaDeathState =
		plazaView.GetComponent<PlayerDeathStateComp>(plazaPlayer);
	assert(plazaStats != nullptr && plazaStats->currentHp == 0);
	assert(plazaDeathState != nullptr);
	assert(plazaDeathState->state ==
		PlayerDeathState::AwaitingRespawnInput);
	assert(plazaDeathState->respawnRequested);
	assert(plazaRuntime.FlushFrameCommands());
	assert(!plazaRuntime.MakeView().IsAlive(plazaPlayer));
	assert(plazaRuntime.FlushLifecycleCommands());
	plazaRuntime.Shutdown();

	GameDataCatalog::Clear();
	GameplayContentCatalogSnapshot::ClearCurrent();

	std::cout << "[PASS] RespawnSystem smoke\n";
}
