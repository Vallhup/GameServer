#include "pch.h"
#include "ServerAppTest.h"
#include "ServerApp.h"

#include <chrono>
#include <iostream>
#include <thread>

#include "ActionDef.h"
#include "BuffDef.h"
#include "CharacterDef.h"
#include "CharacterIdPolicy.h"
#include "DefFileFormat.h"
#include "DefRegistry.h"
#include "AnimationDef.h"
#include "FrameworkRuntime.h"
#include "PlayerEntryService.h"
#include "RepComponent.h"
#include "ServerWorldBootstrap.h"
#include "SessionBindingRegistry.h"
#include "WorldInstance.h"

namespace
{
	using namespace std::chrono_literals;

	enum class TestDefId : uint8_t
	{
		None = 0,
		A = 1,
		B = 2
	};

	struct TestDefEntry
	{
		TestDefId id;
		int value;
	};

	struct TestDefTraits
	{
		static TestDefId GetId(const TestDefEntry& def) noexcept
		{
			return def.id;
		}
	};

	bool WaitUntilRunning(const ServerApp& app, std::chrono::milliseconds timeout)
	{
		const auto deadline = std::chrono::steady_clock::now() + timeout;
		while (std::chrono::steady_clock::now() < deadline)
		{
			if (app.IsRunning())
			{
				return true;
			}

			std::this_thread::sleep_for(1ms);
		}

		return app.IsRunning();
	}

	bool WaitUntilTicked(
		const ServerApp& app,
		uint64_t minTickCount,
		std::chrono::milliseconds timeout)
	{
		const auto deadline = std::chrono::steady_clock::now() + timeout;
		while (std::chrono::steady_clock::now() < deadline)
		{
			if (app.TickCount() >= minTickCount)
			{
				return true;
			}

			std::this_thread::sleep_for(1ms);
		}

		return app.TickCount() >= minTickCount;
	}

	bool PromoteStartupWorldToRunning(FrameworkRuntime& framework)
	{
		return framework.TickServices(0.0, 1.0 / 30.0) &&
			framework.TickServices(1.0 / 30.0, 1.0 / 30.0);
	}

	bool RunSingleAppLikeFrame(
		FrameworkRuntime& framework,
		uint64_t frameIndex,
		double nowSec,
		double dtSec,
		FrameworkRuntime::FrameResult& outFrame)
	{
		if (!framework.TickServices(nowSec, dtSec))
		{
			return false;
		}

		return framework.RunFrame(
			FrameworkRuntime::FrameParams{
				.frameIndex = frameIndex,
				.nowSec = nowSec,
				.dtSec = dtSec
			},
			outFrame);
	}

	void PrintRuntimeState(const char* label, const WorldRuntime& runtime)
	{
		const WorldRuntimeFault& fault = runtime.GetFault();
		std::cout
			<< "[ServerAppTest] " << label
			<< " lifecycleState=" << static_cast<int>(runtime.GetLifecycleState())
			<< " commitState=" << static_cast<int>(runtime.GetCommitState())
			<< " flushState=" << static_cast<int>(runtime.GetLifecycleFlushState())
			<< " frameIndex=" << runtime.FrameIndex()
			<< " isFaulted=" << runtime.IsFaulted()
			<< " faultCode=" << static_cast<int>(fault.code)
			<< " faultMessage=" << fault.message
			<< "\n";
	}
}

// Manual smoke test helper.
// Invoke this from main or a debugger watch when you want a quick end-to-end sanity check
// that ServerApp can initialize, enter the run loop, tick at least once, and shut down cleanly.
bool RunServerAppSmokeTest()
{
	ServerApp::Config config{};
	config.listenPort = 7010;
	config.logicTickHz = 30;
	config.networkThreadCount = 1;
	config.executorWorkerCount = 1;

	ServerApp app(config);

	if (!app.Initialize())
	{
		std::cout << "[ServerAppTest] Initialize failed.\n";
		return false;
	}

	const AnimationClipDef* const impMelee1 =
		app.GetAnimationRegistry().Find(AnimationId::Imp_Melee_1);
	if (impMelee1 == nullptr ||
		impMelee1->clipId != "Imp_melee_1" ||
		impMelee1->skeleton != "Imp" ||
		impMelee1->fps <= 0.0f ||
		impMelee1->numFrames == 0 ||
		impMelee1->frames.empty() ||
		impMelee1->capsuleDefs.empty())
	{
		std::cout << "[ServerAppTest] Animation registry did not load Imp_melee1.\n";
		app.Shutdown();
		return false;
	}

	std::thread runner([&app]()
	{
		app.Run();
	});

	const bool runningObserved = WaitUntilRunning(app, 250ms);
	const bool tickObserved = WaitUntilTicked(app, 1, 1000ms);

	app.Stop();
	if (runner.joinable())
	{
		runner.join();
	}

	const bool initializedBeforeShutdown = app.IsInitialized();
	const uint64_t tickCount = app.TickCount();

	app.Shutdown();

	if (!runningObserved)
	{
		std::cout << "[ServerAppTest] Run loop did not enter running state.\n";
		return false;
	}

	if (!tickObserved || tickCount == 0)
	{
		std::cout << "[ServerAppTest] No logic tick was observed.\n";
		return false;
	}

	if (!initializedBeforeShutdown)
	{
		std::cout << "[ServerAppTest] App lost initialized state before shutdown.\n";
		return false;
	}

	if (app.IsInitialized())
	{
		std::cout << "[ServerAppTest] Shutdown did not clear initialized state.\n";
		return false;
	}

	std::cout << "[ServerAppTest] Passed. ticks=" << tickCount << "\n";
	return true;
}

// Focused flow test:
// - framework bootstrap / startup world ready
// - deferred replicated entity spawn
// - authoritative NetId harvest via FrameResult.events
// - session binding registry bind / lookup
// - deferred despawn and NetId release
bool RunServerAppLoginSpawnFlowTest()
{
	const DefFileHeader testHeader =
		MakeDefFileHeader(DefTypeTag::Animation, DefDataEncoding::Binary, 2, 64);
	if (!IsValidDefFileHeader(testHeader) ||
		testHeader.typeTag != DefTypeTag::Animation ||
		testHeader.encoding != DefDataEncoding::Binary ||
		testHeader.entryCount != 2 ||
		testHeader.payloadSizeBytes != 64)
	{
		std::cout << "[ServerAppTest] DefFileHeader is invalid.\n";
		return false;
	}

	DefRegistry<TestDefEntry, TestDefId, TestDefTraits> testRegistry;
	if (!testRegistry.Build(
		std::vector<TestDefEntry>
		{
			TestDefEntry{ .id = TestDefId::A, .value = 10 },
			TestDefEntry{ .id = TestDefId::B, .value = 20 }
		}) ||
		testRegistry.Size() != 2 ||
		testRegistry.Find(TestDefId::A) == nullptr ||
		testRegistry.Find(TestDefId::B)->value != 20)
	{
		std::cout << "[ServerAppTest] DefRegistry valid build failed.\n";
		return false;
	}

	std::string duplicateBuildError;
	if (testRegistry.Build(
		std::vector<TestDefEntry>
		{
			TestDefEntry{ .id = TestDefId::A, .value = 10 },
			TestDefEntry{ .id = TestDefId::A, .value = 20 }
		},
		&duplicateBuildError) ||
		duplicateBuildError.empty())
	{
		std::cout << "[ServerAppTest] DefRegistry duplicate detection failed.\n";
		return false;
	}

	const CharacterDef* const knightDef = FindCharacterDef(CharacterId::Knight);
	const CharacterDef* const impDef = FindCharacterDef(CharacterId::Imp);
	if (knightDef == nullptr ||
		impDef == nullptr)
	{
		std::cout << "[ServerAppTest] Required CharacterDefs are missing.\n";
		return false;
	}

	if (knightDef->name != "Knight" ||
		knightDef->profile.faction != Faction::Player ||
		knightDef->ai.has_value() ||
		knightDef->stat.maxHp == 0 ||
		knightDef->stat.maxStamina == 0)
	{
		std::cout << "[ServerAppTest] Knight CharacterDef is invalid.\n";
		return false;
	}

	if (!IsPlayableCharacterId(CharacterId::Knight) ||
		IsPlayableCharacterId(CharacterId::Imp) ||
		!IsMonsterCharacterId(CharacterId::Imp))
	{
		std::cout << "[ServerAppTest] CharacterId policy mismatch.\n";
		return false;
	}

	const ActionDef* const knightLightAttack1 =
		FindActionDef(ActionId::Knight_LightAttack1);
	if (knightLightAttack1 == nullptr ||
		knightLightAttack1->characterId != CharacterId::Knight ||
		knightLightAttack1->kind != ActionKind::Attack ||
		knightLightAttack1->playerInput != PlayerActionInput::LightAttack ||
		knightLightAttack1->comboIndex != 1 ||
		knightLightAttack1->duration <= 0.0f)
	{
		std::cout << "[ServerAppTest] Knight LightAttack1 ActionDef is invalid.\n";
		return false;
	}

	const ActionDef* const knightLightAttack2 =
		FindActionDef(ActionId::Knight_LightAttack2);
	if (knightLightAttack2 == nullptr ||
		knightLightAttack2->characterId != CharacterId::Knight ||
		knightLightAttack2->kind != ActionKind::Attack ||
		knightLightAttack2->playerInput != PlayerActionInput::LightAttack ||
		knightLightAttack2->comboIndex != 2 ||
		knightLightAttack2->duration <= 0.0f)
	{
		std::cout << "[ServerAppTest] Knight LightAttack2 ActionDef is invalid.\n";
		return false;
	}

	const ActionDef* const knightLightAttack3 =
		FindActionDef(ActionId::Knight_LightAttack3);
	if (knightLightAttack3 == nullptr ||
		knightLightAttack3->characterId != CharacterId::Knight ||
		knightLightAttack3->kind != ActionKind::Attack ||
		knightLightAttack3->playerInput != PlayerActionInput::LightAttack ||
		knightLightAttack3->comboIndex != 3 ||
		knightLightAttack3->duration <= 0.0f)
	{
		std::cout << "[ServerAppTest] Knight LightAttack3 ActionDef is invalid.\n";
		return false;
	}

	const ActionDef* const knightHeavyAttack =
		FindActionDef(ActionId::Knight_HeavyAttack);
	if (knightHeavyAttack == nullptr ||
		knightHeavyAttack->characterId != CharacterId::Knight ||
		knightHeavyAttack->kind != ActionKind::Attack ||
		knightHeavyAttack->playerInput != PlayerActionInput::HeavAttack ||
		knightHeavyAttack->duration <= 0.0f)
	{
		std::cout << "[ServerAppTest] Knight HeavyAttack ActionDef is invalid.\n";
		return false;
	}

	const ActionDef* const knightSpecialAttack =
		FindActionDef(ActionId::Knight_SpecialAttack);
	if (knightSpecialAttack == nullptr ||
		knightSpecialAttack->characterId != CharacterId::Knight ||
		knightSpecialAttack->kind != ActionKind::Attack ||
		knightSpecialAttack->duration <= 0.0f ||
		knightSpecialAttack->combatWindows.empty())
	{
		std::cout << "[ServerAppTest] Knight SpecialAttack ActionDef is invalid.\n";
		return false;
	}

	bool hasParrySuccessReadyRequirement = false;
	for (const ActionRequestRequirementDef& requirement : knightSpecialAttack->requestRequirements)
	{
		if (requirement.type == ActionRequestRequirementType::HasStateFlag &&
			requirement.stateFlag == GameplayStateFlag::ParrySuccessReady)
		{
			hasParrySuccessReadyRequirement = true;
			break;
		}
	}
	if (!hasParrySuccessReadyRequirement)
	{
		std::cout << "[ServerAppTest] Knight SpecialAttack is missing ParrySuccessReady requirement.\n";
		return false;
	}

	const BuffDef* const parrySuccessBuff = FindBuffDef(BuffId::ParrySuccess);
	if (parrySuccessBuff == nullptr ||
		parrySuccessBuff->profile.familyId != BuffFamilyId::ParrySuccess ||
		parrySuccessBuff->profile.tier != BuffTier::None ||
		parrySuccessBuff->lifetime.durationPolicy != DurationPolicy::Timed ||
		parrySuccessBuff->effects.empty())
	{
		std::cout << "[ServerAppTest] ParrySuccess BuffDef is invalid.\n";
		return false;
	}

	const BuffEffectDef& parrySuccessEffect = parrySuccessBuff->effects.front();
	if (parrySuccessEffect.type != BuffEffectType::StateFlag ||
		!parrySuccessEffect.stateFlag.has_value() ||
		parrySuccessEffect.stateFlag->flag != GameplayStateFlag::ParrySuccessReady)
	{
		std::cout << "[ServerAppTest] ParrySuccess BuffDef effect is invalid.\n";
		return false;
	}

	bool removesOnSpecialAttack = false;
	bool removesOnFirstAttackCommit = false;
	for (const BuffRemoveRuleDef& removeRule : parrySuccessBuff->removeRules)
	{
		if (removeRule.type == BuffRemoveRuleType::OnCounterReached &&
			removeRule.operand.counterEventCondition == BuffCounterEventType::ActionCommitted &&
			removeRule.operand.actionKindCondition == ActionKind::Attack &&
			removeRule.operand.scalarCondition == 1.0f)
		{
			removesOnFirstAttackCommit = true;
			break;
		}
	}
	if (!removesOnFirstAttackCommit)
	{
		std::cout << "[ServerAppTest] ParrySuccess BuffDef is missing attack-commit consume rule.\n";
		return false;
	}

	const BuffId hpBoostIds[] =
	{
		BuffId::HpBoost_Low,
		BuffId::HpBoost_Mid,
		BuffId::HpBoost_High
	};
	const BuffTier hpBoostTiers[] =
	{
		BuffTier::Low,
		BuffTier::Mid,
		BuffTier::High
	};
	const float hpBoostValues[] = { 10.0f, 20.0f, 30.0f };
	for (size_t index = 0; index < std::size(hpBoostIds); ++index)
	{
		const BuffDef* const hpBoost = FindBuffDef(hpBoostIds[index]);
		if (hpBoost == nullptr ||
			hpBoost->profile.familyId != BuffFamilyId::HpBoost ||
			hpBoost->profile.groupId != BuffFamilyId::HpBoost ||
			hpBoost->profile.familyStackPolicy != BuffFamilyStackPolicy::ReplaceWithHigherTier ||
			hpBoost->profile.tier != hpBoostTiers[index] ||
			hpBoost->effects.size() != 1 ||
			hpBoost->lifetime.durationPolicy != DurationPolicy::Infinite ||
			!hpBoost->removeRules.empty())
		{
			std::cout << "[ServerAppTest] HpBoost BuffDef profile is invalid.\n";
			return false;
		}

		const BuffEffectDef& hpBoostEffect = hpBoost->effects.front();
		if (hpBoostEffect.type != BuffEffectType::StatAdd ||
			!hpBoostEffect.stat.has_value() ||
			hpBoostEffect.stat->statType != StatType::MaxHp ||
			hpBoostEffect.stat->value != hpBoostValues[index])
		{
			std::cout << "[ServerAppTest] HpBoost BuffDef effect is invalid.\n";
			return false;
		}
	}

	const ActionDef* const knightDodge =
		FindActionDef(ActionId::Knight_Dodge);
	if (knightDodge == nullptr ||
		knightDodge->characterId != CharacterId::Knight ||
		knightDodge->kind != ActionKind::Dodge ||
		knightDodge->duration <= 0.0f ||
		knightDodge->moveSegments.empty() ||
		knightDodge->combatWindows.empty())
	{
		std::cout << "[ServerAppTest] Knight Dodge ActionDef is invalid.\n";
		return false;
	}

	const ActionDef* const knightParry =
		FindActionDef(ActionId::Knight_Parry);
	if (knightParry == nullptr ||
		knightParry->characterId != CharacterId::Knight ||
		knightParry->kind != ActionKind::Parry ||
		knightParry->playerInput != PlayerActionInput::Parry ||
		knightParry->duration <= 0.0f ||
		knightParry->combatWindows.empty())
	{
		std::cout << "[ServerAppTest] Knight Parry ActionDef is invalid.\n";
		return false;
	}

	const ActionCombatWindowDef& knightParryWindow = knightParry->combatWindows.front();
	if (knightParryWindow.windowType != CombatWindowType::Parry ||
		knightParryWindow.appliesTo != ActionCombatApplyTo::ParryableAttack ||
		!knightParryWindow.spatialFilter.has_value() ||
		!knightParryWindow.effect.has_value() ||
		knightParryWindow.effect->type != CombatEffectType::ParryResponse ||
		!knightParryWindow.effect->parryResponse.has_value() ||
		knightParryWindow.effect->parryResponse->grantBuffId != BuffId::ParrySuccess ||
		knightParryWindow.spatialFilter->facingHalfAngleDeg != 45.0f ||
		knightParryWindow.spatialFilter->referenceFrame != CombatReferenceFrame::OwnerFacing)
	{
		std::cout << "[ServerAppTest] Knight Parry combat window is invalid.\n";
		return false;
	}

	const ActionDef* const knightGuard =
		FindActionDef(ActionId::Knight_Guard);
	if (knightGuard == nullptr ||
		knightGuard->characterId != CharacterId::Knight ||
		knightGuard->kind != ActionKind::Guard ||
		knightGuard->playerInput != PlayerActionInput::Guard ||
		knightGuard->normalizedPolicy != ActionNormalizedPolicy::Holdable ||
		knightGuard->endPolicy.endType != ActionEndType::HoldRelease ||
		knightGuard->combatWindows.empty())
	{
		std::cout << "[ServerAppTest] Knight Guard ActionDef is invalid.\n";
		return false;
	}

	const ActionCombatWindowDef& knightGuardWindow = knightGuard->combatWindows.front();
	if (knightGuardWindow.windowType != CombatWindowType::Guard ||
		knightGuardWindow.appliesTo != ActionCombatApplyTo::GuardableAttack ||
		!knightGuardWindow.spatialFilter.has_value() ||
		!knightGuardWindow.effect.has_value() ||
		knightGuardWindow.effect->type != CombatEffectType::GuardResponse ||
		knightGuardWindow.spatialFilter->facingHalfAngleDeg != 60.0f ||
		knightGuardWindow.spatialFilter->referenceFrame != CombatReferenceFrame::OwnerFacing)
	{
		std::cout << "[ServerAppTest] Knight Guard combat window is invalid.\n";
		return false;
	}

	const ActionDef* const knightStun =
		FindActionDef(ActionId::Knight_Stun);
	if (knightStun == nullptr ||
		knightStun->characterId != CharacterId::Knight ||
		knightStun->kind != ActionKind::Stun ||
		knightStun->playerInput != PlayerActionInput::None ||
		knightStun->duration <= 0.0f)
	{
		std::cout << "[ServerAppTest] Knight Stun ActionDef is invalid.\n";
		return false;
	}

	const ActionDef* const knightHit =
		FindActionDef(ActionId::Knight_Hit);
	if (knightHit == nullptr ||
		knightHit->characterId != CharacterId::Knight ||
		knightHit->kind != ActionKind::Hit ||
		knightHit->playerInput != PlayerActionInput::None ||
		knightHit->duration <= 0.0f)
	{
		std::cout << "[ServerAppTest] Knight Hit ActionDef is invalid.\n";
		return false;
	}

	const ActionDef* const knightDead =
		FindActionDef(ActionId::Knight_Dead);
	if (knightDead == nullptr ||
		knightDead->characterId != CharacterId::Knight ||
		knightDead->kind != ActionKind::Dead ||
		knightDead->playerInput != PlayerActionInput::None ||
		knightDead->duration <= 0.0f)
	{
		std::cout << "[ServerAppTest] Knight Dead ActionDef is invalid.\n";
		return false;
	}

	const ActionId impActionIds[] =
	{
		ActionId::Imp_melee1,
		ActionId::Imp_melee2,
		ActionId::Imp_melee3,
		ActionId::Imp_melee4,
		ActionId::Imp_melee5,
		ActionId::Imp_Stun,
		ActionId::Imp_Hit,
		ActionId::Imp_Dead
	};
	for (const ActionId actionId : impActionIds)
	{
		const ActionDef* const impAction = FindActionDef(actionId);
		if (impAction == nullptr ||
			impAction->characterId != CharacterId::Imp ||
			impAction->duration <= 0.0f)
		{
			std::cout << "[ServerAppTest] Imp ActionDef is invalid.\n";
			return false;
		}
	}

	ServerWorldBootstrapFactory bootstrapFactory;
	ServerWorldBootstrapDefinitionProvider bootstrapDefinitions;

	FrameworkRuntime framework(FrameworkRuntime::Config{
		.executorWorkerCount = 1
	});

	FrameworkRuntime::BootstrapParams bootstrapParams{};
	bootstrapParams.worldFactory = &bootstrapFactory;
	bootstrapParams.definitionProvider = &bootstrapDefinitions;

	if (!framework.Initialize(bootstrapParams))
	{
		std::cout << "[ServerAppTest] Framework initialize failed.\n";
		return false;
	}

	const WorldId startupWorldId =
		framework.RegisterPreCreatedWorld(WorldDefId::Square, 0);
	if (!startupWorldId.IsValid())
	{
		std::cout << "[ServerAppTest] Startup world registration failed.\n";
		framework.Shutdown();
		return false;
	}

	if (!framework.InitializeWorld(startupWorldId))
	{
		std::cout << "[ServerAppTest] Startup world initialize failed.\n";
		framework.Shutdown();
		return false;
	}

	WorldInstance* world = framework.FindWorld(startupWorldId);
	if (world == nullptr)
	{
		std::cout << "[ServerAppTest] Startup world lookup failed.\n";
		framework.Shutdown();
		return false;
	}

	WorldRuntime& runtime = world->GetRuntime();
	PlayerEntryService entryService(PlayerEntryService::Dependencies{
		&framework,
		&startupWorldId
	});

	FrameworkRuntime::FrameResult warmupFrame1{};
	if (!RunSingleAppLikeFrame(framework, 1, 1.0 / 30.0, 1.0 / 30.0, warmupFrame1))
	{
		std::cout << "[ServerAppTest] Warmup frame 1 failed.\n";
		PrintRuntimeState("Warmup1", runtime);
		framework.Shutdown();
		return false;
	}

	FrameworkRuntime::FrameResult warmupFrame2{};
	if (!RunSingleAppLikeFrame(framework, 2, 2.0 / 30.0, 1.0 / 30.0, warmupFrame2))
	{
		std::cout << "[ServerAppTest] Warmup frame 2 failed.\n";
		PrintRuntimeState("Warmup2", runtime);
		framework.Shutdown();
		return false;
	}

	constexpr SessionId testSessionId = 1001;
	if (!entryService.BeginAuthenticatedEntry(testSessionId))
	{
		std::cout << "[ServerAppTest] BeginAuthenticatedEntry failed.\n";
		framework.Shutdown();
		return false;
	}

	if (!entryService.IsAwaitingCharacterSelect(testSessionId))
	{
		std::cout << "[ServerAppTest] Session should await character selection.\n";
		framework.Shutdown();
		return false;
	}

	const PlayerEntryResult impSelect =
		entryService.RequestCharacterSelect(testSessionId, CharacterId::Imp);
	if (impSelect.code != PlayerEntryResultCode::CharacterIdNotPlayable)
	{
		std::cout << "[ServerAppTest] Non-playable CharacterId was not rejected.\n";
		framework.Shutdown();
		return false;
	}

	const PlayerEntryResult knightSelect =
		entryService.RequestCharacterSelect(testSessionId, CharacterId::Knight);
	if (!knightSelect.Succeeded() ||
		knightSelect.worldId != startupWorldId ||
		knightSelect.entity.IsNull() ||
		knightSelect.characterId != CharacterId::Knight)
	{
		std::cout << "[ServerAppTest] Knight character selection failed.\n";
		framework.Shutdown();
		return false;
	}

	if (!entryService.IsSpawnPending(testSessionId) ||
		entryService.FindPendingSpawn(testSessionId) == nullptr)
	{
		std::cout << "[ServerAppTest] Spawn should be pending after character select.\n";
		framework.Shutdown();
		return false;
	}

	FrameworkRuntime::FrameResult spawnFrame{};
	if (!RunSingleAppLikeFrame(
		framework,
		3,
		3.0 / 30.0,
		1.0 / 30.0,
		spawnFrame))
	{
		std::cout << "[ServerAppTest] Spawn frame failed.\n";
		PrintRuntimeState("Spawn", runtime);
		framework.Shutdown();
		return false;
	}

	if (spawnFrame.events.spawns.size() != 1)
	{
		std::cout << "[ServerAppTest] Expected one spawn event, got "
			<< spawnFrame.events.spawns.size() << ".\n";
		std::cout << "[ServerAppTest] Spawn frame selectedWorldCount="
			<< spawnFrame.selectedWorldCount << ".\n";
		PrintRuntimeState("SpawnNoEvent", runtime);
		framework.Shutdown();
		return false;
	}

	const auto& spawnEvent = spawnFrame.events.spawns.front();
	if (spawnEvent.worldId != startupWorldId ||
		spawnEvent.entity != knightSelect.entity ||
		!spawnEvent.netId.IsValid())
	{
		std::cout << "[ServerAppTest] Spawn event contents are invalid.\n";
		framework.Shutdown();
		return false;
	}

	PendingCharacterSpawn confirmedSpawn{};
	if (!entryService.TryConsumeSpawnConfirmed(
		spawnEvent.worldId,
		spawnEvent.entity,
		confirmedSpawn))
	{
		std::cout << "[ServerAppTest] Pending spawn confirm failed.\n";
		framework.Shutdown();
		return false;
	}

	if (confirmedSpawn.sessionId != testSessionId ||
		confirmedSpawn.characterId != CharacterId::Knight ||
		confirmedSpawn.entity != knightSelect.entity)
	{
		std::cout << "[ServerAppTest] Confirmed spawn contents are invalid.\n";
		framework.Shutdown();
		return false;
	}

	if (entryService.FindContext(testSessionId) != nullptr ||
		entryService.FindPendingSpawn(testSessionId) != nullptr)
	{
		std::cout << "[ServerAppTest] Entry context was not cleared after spawn confirm.\n";
		framework.Shutdown();
		return false;
	}

	if (framework.FindNetId(startupWorldId, knightSelect.entity) != spawnEvent.netId)
	{
		std::cout << "[ServerAppTest] Framework NetId lookup mismatch after spawn.\n";
		framework.Shutdown();
		return false;
	}

	SessionBindingRegistry sessionBindings;
	if (!sessionBindings.Bind(testSessionId, spawnEvent.netId, startupWorldId))
	{
		std::cout << "[ServerAppTest] Session binding failed.\n";
		framework.Shutdown();
		return false;
	}

	if (!sessionBindings.HasBinding(testSessionId) ||
		sessionBindings.FindControlledNetId(testSessionId) != spawnEvent.netId ||
		sessionBindings.FindOwnerSession(spawnEvent.netId) != testSessionId ||
		sessionBindings.FindCurrentWorldId(testSessionId) != startupWorldId)
	{
		std::cout << "[ServerAppTest] Session binding lookup mismatch.\n";
		framework.Shutdown();
		return false;
	}

	constexpr SessionId cancelSessionId = 1002;
	if (!entryService.BeginAuthenticatedEntry(cancelSessionId))
	{
		std::cout << "[ServerAppTest] Cancel test entry begin failed.\n";
		framework.Shutdown();
		return false;
	}

	const PlayerEntryResult cancelSelect =
		entryService.RequestCharacterSelect(cancelSessionId, CharacterId::Knight);
	if (!cancelSelect.Succeeded())
	{
		std::cout << "[ServerAppTest] Cancel test character selection failed.\n";
		framework.Shutdown();
		return false;
	}

	if (!entryService.CancelEntry(cancelSessionId))
	{
		std::cout << "[ServerAppTest] CancelEntry failed.\n";
		framework.Shutdown();
		return false;
	}

	if (entryService.FindContext(cancelSessionId) != nullptr ||
		entryService.FindPendingSpawn(cancelSessionId) != nullptr)
	{
		std::cout << "[ServerAppTest] CancelEntry did not clear entry state.\n";
		framework.Shutdown();
		return false;
	}

	FrameworkRuntime::FrameResult cancelCleanupFrame{};
	if (!RunSingleAppLikeFrame(
		framework,
		4,
		4.0 / 30.0,
		1.0 / 30.0,
		cancelCleanupFrame))
	{
		std::cout << "[ServerAppTest] Cancel cleanup frame failed.\n";
		PrintRuntimeState("CancelCleanup", runtime);
		framework.Shutdown();
		return false;
	}

	if (framework.FindNetId(startupWorldId, cancelSelect.entity).IsValid())
	{
		std::cout << "[ServerAppTest] Cancelled pending entity survived cleanup.\n";
		framework.Shutdown();
		return false;
	}

	runtime.DeferredDestroyEntity(knightSelect.entity);

	FrameworkRuntime::FrameResult despawnFrame{};
	if (!RunSingleAppLikeFrame(
		framework,
		5,
		5.0 / 30.0,
		1.0 / 30.0,
		despawnFrame))
	{
		std::cout << "[ServerAppTest] Despawn frame failed.\n";
		PrintRuntimeState("Despawn", runtime);
		framework.Shutdown();
		return false;
	}

	const auto despawnIt = std::find_if(
		despawnFrame.events.despawns.begin(),
		despawnFrame.events.despawns.end(),
		[&](const FrameworkRuntime::FrameResult::EntityDespawnEvent& event)
		{
			return event.worldId == startupWorldId &&
				event.entity == knightSelect.entity &&
				event.netId == spawnEvent.netId;
		});

	if (despawnIt == despawnFrame.events.despawns.end())
	{
		std::cout << "[ServerAppTest] Knight despawn event was not found.\n";
		PrintRuntimeState("DespawnNoEvent", runtime);
		framework.Shutdown();
		return false;
	}

	const auto& despawnEvent = *despawnIt;
	if (despawnEvent.worldId != startupWorldId ||
		despawnEvent.entity != knightSelect.entity ||
		despawnEvent.netId != spawnEvent.netId)
	{
		std::cout << "[ServerAppTest] Despawn event contents are invalid.\n";
		framework.Shutdown();
		return false;
	}

	if (framework.FindNetId(startupWorldId, knightSelect.entity).IsValid() ||
		framework.IsNetIdAlive(spawnEvent.netId))
	{
		std::cout << "[ServerAppTest] NetId was not released after despawn.\n";
		framework.Shutdown();
		return false;
	}

	(void)sessionBindings.UnbindByNetId(spawnEvent.netId);
	if (sessionBindings.HasBinding(testSessionId))
	{
		std::cout << "[ServerAppTest] Session binding was not cleared.\n";
		framework.Shutdown();
		return false;
	}

	framework.Shutdown();
	std::cout << "[ServerAppTest] LoginSpawnFlow passed.\n";
	return true;
}
