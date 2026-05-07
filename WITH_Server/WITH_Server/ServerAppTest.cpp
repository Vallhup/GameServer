#include "pch.h"
#include "ServerAppTest.h"
#include "ServerApp.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <span>
#include <thread>

#include "CharacterDef.h"
#include "CharacterDataService.h"
#include "CharacterIdPolicy.h"
#include "CharacterSpawnService.h"
#include "DefFileFormat.h"
#include "DefRegistry.h"
#include "AnimationDef.h"
#include "ECS/GameplayRuntimeComponents.h"
#include "FrameworkRuntime.h"
#include "IWorldTransitionRequestSink.h"
#include "NetworkRuntime.h"
#include "RepComponent.h"
#include "ServerFrameEventDispatcher.h"
#include "ServerSessionSystem.h"
#include "ServerWorldBootstrap.h"
#include "ServerWorldTransferBinding.h"
#include "ServerWorldTransferCommitter.h"
#include "SessionBindingRegistry.h"
#include "SessionFlowCommands.h"
#include "SessionFlowController.h"
#include "WorldDef.h"
#include "WorldInstance.h"
#include "WorldInstanceRecord.h"

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

	class NoopWorldTransitionRequestSink final : public IWorldTransitionRequestSink
	{
	public:
		TransferId RequestDemoWorldTransition(
			SessionId sessionId,
			uint32_t requestId) override
		{
			(void)sessionId;
			(void)requestId;
			return 0;
		}

		bool MarkClientWorldTransitionReady(
			SessionId sessionId,
			TransferId transferId) override
		{
			(void)sessionId;
			(void)transferId;
			return false;
		}
	};

	bool NearlyEqual(float lhs, float rhs) noexcept
	{
		return std::abs(lhs - rhs) <= 0.001f;
	}

	bool TryGetDefaultPlayerSpawnPosition(
		const WorldDef& worldDef,
		DirectX::XMFLOAT3& outPosition) noexcept
	{
		if (worldDef.map.defaultPlayerSpawnPointId == SpawnPointIds::None)
		{
			return false;
		}

		for (const SpawnPointDef& spawnPoint : worldDef.map.spawnPoints)
		{
			if (spawnPoint.id != worldDef.map.defaultPlayerSpawnPointId)
			{
				continue;
			}

			outPosition = DirectX::XMFLOAT3{
				spawnPoint.position.x,
				spawnPoint.position.y,
				spawnPoint.position.z
			};
			return true;
		}

		return false;
	}

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

	bool DispatchFrameEvents(
		const FrameworkRuntime::FrameResult& frameResult,
		FrameworkRuntime& framework,
		ServerSessionSystem& sessionSystem,
		double nowSec)
	{
		return ServerFrameEventDispatcher::Dispatch(
			frameResult,
			framework,
			sessionSystem,
			nowSec);
	}

	bool RunAppLikeFrame(
		FrameworkRuntime& framework,
		ServerSessionSystem& sessionSystem,
		uint64_t frameIndex,
		double nowSec,
		double dtSec,
		FrameworkRuntime::FrameResult& outFrame)
	{
		if (!framework.TickServices(nowSec, dtSec))
		{
			std::cout << "[WorldTransitionSmoke] TickServices failed.\n";
			return false;
		}

		if (!ServerWorldTransferCommitter::Commit(
			framework,
			sessionSystem.Bindings()))
		{
			std::cout << "[WorldTransitionSmoke] transfer commit failed.\n";
			return false;
		}

		if (!framework.RunFrame(
			FrameworkRuntime::FrameParams{
				.frameIndex = frameIndex,
				.nowSec = nowSec,
				.dtSec = dtSec
			},
			outFrame))
		{
			std::cout << "[WorldTransitionSmoke] RunFrame failed."
				<< " frameIndex=" << frameIndex
				<< " reason=" << static_cast<int>(outFrame.failureReason)
				<< "\n";
			return false;
		}

		return DispatchFrameEvents(
			outFrame,
			framework,
			sessionSystem,
			nowSec);
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

	/*const AnimationClipDef* const impMelee1 =
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
	}*/

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

bool RunWorldTransitionDebugSmokeTest()
{
	constexpr SessionId kSessionId = 1;
	constexpr double kDtSec = 1.0 / 30.0;

	AnimationRegistry animationRegistry;
	FrameworkRuntime framework(FrameworkRuntime::Config{
		.executorWorkerCount = 1
	});
	NoopWorldTransitionRequestSink worldTransitionSink;
	WorldId startupWorldId = WorldId::Invalid();
	ServerSessionSystem sessionSystem(
		ServerSessionSystem::Config{
			.networkThreadCount = 1,
			.listenPort = 7011,
			.maxSessions = 8
		},
		framework,
		startupWorldId,
		worldTransitionSink);
	ServerWorldTransferBinding transferBinding(
		framework,
		sessionSystem.Bindings());
	ServerWorldBootstrapFactory bootstrapFactory;
	ServerWorldBootstrapDefinitionProvider bootstrapDefinitions;

	bootstrapFactory.SetAnimationRegistry(&animationRegistry);
	bootstrapFactory.SetFramework(&framework);
	bootstrapFactory.SetBootstrapWorldId(&startupWorldId);

	FrameworkRuntime::BootstrapParams bootstrapParams{};
	bootstrapParams.worldFactory = &bootstrapFactory;
	bootstrapParams.definitionProvider = &bootstrapDefinitions;
	bootstrapParams.transferBinding = &transferBinding;

	if (!framework.Initialize(bootstrapParams))
	{
		std::cout << "[WorldTransitionSmoke] framework initialize failed.\n";
		return false;
	}

	startupWorldId =
		framework.RegisterPreCreatedWorld(WorldDefId::Plaza, 0);
	if (!startupWorldId.IsValid() || !framework.InitializeWorld(startupWorldId))
	{
		std::cout << "[WorldTransitionSmoke] startup world initialize failed.\n";
		framework.Shutdown();
		return false;
	}

	if (!PromoteStartupWorldToRunning(framework))
	{
		std::cout << "[WorldTransitionSmoke] startup world did not become running.\n";
		framework.Shutdown();
		return false;
	}

	if (!sessionSystem.Initialize())
	{
		std::cout << "[WorldTransitionSmoke] session system initialize failed.\n";
		framework.Shutdown();
		return false;
	}

	SessionFlowResult flowResult =
		sessionSystem.Flow().Dispatch(kSessionId, LoginRequested{});
	if (!flowResult.Succeeded())
	{
		std::cout << "[WorldTransitionSmoke] login request flow failed.\n";
		sessionSystem.Shutdown();
		framework.Shutdown();
		return false;
	}

	const NetId reservedNetId = framework.AllocateNetId();
	LoginSucceeded loginSucceeded{};
	loginSucceeded.playerNetId = reservedNetId;
	flowResult = sessionSystem.Flow().Dispatch(kSessionId, loginSucceeded);
	if (!flowResult.Succeeded())
	{
		std::cout << "[WorldTransitionSmoke] login success flow failed.\n";
		sessionSystem.Shutdown();
		framework.Shutdown();
		return false;
	}

	CharacterSelectRequested selectRequested{};
	selectRequested.characterId = CharacterId::Knight;
	flowResult = sessionSystem.Flow().Dispatch(kSessionId, selectRequested);
	if (!flowResult.Succeeded())
	{
		std::cout << "[WorldTransitionSmoke] character select flow failed.\n";
		sessionSystem.Shutdown();
		framework.Shutdown();
		return false;
	}

	const CharacterDataResult dataResult =
		sessionSystem.CharacterData().ResolveCharacterSelect(kSessionId, CharacterId::Knight);
	if (!dataResult.Succeeded())
	{
		std::cout << "[WorldTransitionSmoke] character data failed."
			<< " code=" << static_cast<int>(dataResult.code)
			<< "\n";
		sessionSystem.Shutdown();
		framework.Shutdown();
		return false;
	}

	CharacterDataLoaded dataLoaded{};
	dataLoaded.characterId = dataResult.characterId;
	dataLoaded.worldId = dataResult.worldId;
	flowResult = sessionSystem.Flow().Dispatch(kSessionId, dataLoaded);
	if (!flowResult.Succeeded())
	{
		std::cout << "[WorldTransitionSmoke] character data flow failed.\n";
		sessionSystem.Shutdown();
		framework.Shutdown();
		return false;
	}

	const CharacterSpawnResult spawnResult =
		sessionSystem.CharacterSpawn().RequestCharacterSpawn(dataResult, reservedNetId);
	if (!spawnResult.Succeeded())
	{
		std::cout << "[WorldTransitionSmoke] character spawn failed."
			<< " code=" << static_cast<int>(spawnResult.code)
			<< "\n";
		sessionSystem.Shutdown();
		framework.Shutdown();
		return false;
	}

	double nowSec = 0.0;
	uint64_t frameIndex = 1;
	FrameworkRuntime::FrameResult frameResult{};

	sessionSystem.BeginSendStage();
	nowSec += kDtSec;
	if (!RunAppLikeFrame(
		framework,
		sessionSystem,
		frameIndex++,
		nowSec,
		kDtSec,
		frameResult))
	{
		sessionSystem.Shutdown();
		framework.Shutdown();
		return false;
	}

	if (sessionSystem.MarkInitialWorldReady(
			kSessionId,
			1,
			nowSec) != InitialWorldReadyResult::Accepted)
	{
		std::cout << "[WorldTransitionSmoke] initial world ready failed.\n";
		sessionSystem.Shutdown();
		framework.Shutdown();
		return false;
	}

	const SessionBinding* loginBinding =
		sessionSystem.Bindings().FindBySession(kSessionId);
	if (loginBinding == nullptr ||
		loginBinding->currentWorldId != startupWorldId ||
		!loginBinding->controlledNetId.IsValid())
	{
		std::cout << "[WorldTransitionSmoke] login binding was not established.\n";
		sessionSystem.Shutdown();
		framework.Shutdown();
		return false;
	}

	const NetId playerNetId = loginBinding->controlledNetId;
	const NetBindingLocation sourceLocation =
		framework.FindNetBinding(playerNetId);
	if (!sourceLocation.IsValid() || sourceLocation.worldId != startupWorldId)
	{
		std::cout << "[WorldTransitionSmoke] source net binding invalid after login.\n";
		sessionSystem.Shutdown();
		framework.Shutdown();
		return false;
	}

	WorldInstance* const startupWorld = framework.FindWorld(startupWorldId);
	if (startupWorld == nullptr || startupWorld->GetDef() == nullptr)
	{
		std::cout << "[WorldTransitionSmoke] startup world definition unavailable.\n";
		sessionSystem.Shutdown();
		framework.Shutdown();
		return false;
	}

	DirectX::XMFLOAT3 expectedSpawnPosition{};
	if (!TryGetDefaultPlayerSpawnPosition(
		*startupWorld->GetDef(),
		expectedSpawnPosition))
	{
		std::cout << "[WorldTransitionSmoke] startup world player spawn point missing.\n";
		sessionSystem.Shutdown();
		framework.Shutdown();
		return false;
	}

	const WorldTransformComp* const playerTransform =
		startupWorld->GetRuntime().MakeView().GetComponent<WorldTransformComp>(
			sourceLocation.entity);
	if (playerTransform == nullptr)
	{
		std::cout << "[WorldTransitionSmoke] player transform missing after login.\n";
		sessionSystem.Shutdown();
		framework.Shutdown();
		return false;
	}

	if (!NearlyEqual(playerTransform->position.x, expectedSpawnPosition.x) ||
		!NearlyEqual(playerTransform->position.y, expectedSpawnPosition.y) ||
		!NearlyEqual(playerTransform->position.z, expectedSpawnPosition.z))
	{
		std::cout << "[WorldTransitionSmoke] player did not spawn at default world spawn point."
			<< " actual=("
			<< playerTransform->position.x << ", "
			<< playerTransform->position.y << ", "
			<< playerTransform->position.z << ")"
			<< " expected=("
			<< expectedSpawnPosition.x << ", "
			<< expectedSpawnPosition.y << ", "
			<< expectedSpawnPosition.z << ")\n";
		sessionSystem.Shutdown();
		framework.Shutdown();
		return false;
	}

	const SessionId sessions[] = { kSessionId };
	const TransferId transferId = framework.RequestWorldTransfer(
		std::span<const SessionId>(sessions, 1),
		startupWorldId,
		WorldDefId::Village,
		static_cast<uint64_t>(kSessionId),
		static_cast<PartyId>(kSessionId),
		true,
		nowSec);
	if (transferId == 0)
	{
		std::cout << "[WorldTransitionSmoke] transfer request was rejected.\n";
		sessionSystem.Shutdown();
		framework.Shutdown();
		return false;
	}

	for (uint32_t step = 0; step < 12; ++step)
	{
		sessionSystem.BeginSendStage();
		nowSec += kDtSec;
		if (!RunAppLikeFrame(
			framework,
			sessionSystem,
			frameIndex++,
			nowSec,
			kDtSec,
			frameResult))
		{
			sessionSystem.Shutdown();
			framework.Shutdown();
			return false;
		}
	}

	const SessionBinding* transferBindingResult =
		sessionSystem.Bindings().FindBySession(kSessionId);
	if (transferBindingResult == nullptr ||
		transferBindingResult->controlledNetId != playerNetId ||
		transferBindingResult->currentWorldId == startupWorldId)
	{
		std::cout << "[WorldTransitionSmoke] session binding did not move to target.\n";
		sessionSystem.Shutdown();
		framework.Shutdown();
		return false;
	}

	const WorldInstanceRecord* targetRecord =
		framework.FindWorldRecord(transferBindingResult->currentWorldId);
	if (targetRecord == nullptr ||
		targetRecord->defId != WorldDefId::Village ||
		!targetRecord->IsRunnable() ||
		targetRecord->activePlayers != 1)
	{
		std::cout << "[WorldTransitionSmoke] target world record invalid."
			<< " worldId=" << transferBindingResult->currentWorldId.GetRaw()
			<< "\n";
		sessionSystem.Shutdown();
		framework.Shutdown();
		return false;
	}

	const WorldInstanceRecord* sourceRecord =
		framework.FindWorldRecord(startupWorldId);
	if (sourceRecord == nullptr || sourceRecord->activePlayers != 0)
	{
		std::cout << "[WorldTransitionSmoke] source active player count invalid.\n";
		sessionSystem.Shutdown();
		framework.Shutdown();
		return false;
	}

	const NetBindingLocation targetLocation =
		framework.FindNetBinding(playerNetId);
	if (!targetLocation.IsValid() ||
		targetLocation.worldId != transferBindingResult->currentWorldId ||
		framework.FindNetId(startupWorldId, sourceLocation.entity).IsValid())
	{
		std::cout << "[WorldTransitionSmoke] net binding did not move to target.\n";
		sessionSystem.Shutdown();
		framework.Shutdown();
		return false;
	}

	WorldInstance* targetWorld =
		framework.FindWorld(transferBindingResult->currentWorldId);
	if (targetWorld == nullptr ||
		!targetWorld->GetRuntime().MakeView().IsAlive(targetLocation.entity))
	{
		std::cout << "[WorldTransitionSmoke] target entity is not alive.\n";
		sessionSystem.Shutdown();
		framework.Shutdown();
		return false;
	}

	std::cout << "[WorldTransitionSmoke] Passed."
		<< " transferId=" << transferId
		<< " sourceWorldId=" << startupWorldId.GetRaw()
		<< " targetWorldId=" << transferBindingResult->currentWorldId.GetRaw()
		<< " netId=" << playerNetId.GetRaw()
		<< "\n";

	sessionSystem.Shutdown();
	framework.Shutdown();
	return true;
}
