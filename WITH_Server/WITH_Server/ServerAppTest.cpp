#include "pch.h"
#include "ServerAppTest.h"
#include "ServerApp.h"

#include <chrono>
#include <iostream>
#include <span>
#include <thread>

#include "ActionDef.h"
#include "BuffDef.h"
#include "CharacterDef.h"
#include "CharacterIdPolicy.h"
#include "DefFileFormat.h"
#include "DefRegistry.h"
#include "AnimationDef.h"
#include "FrameworkRuntime.h"
#include "NetworkRuntime.h"
#include "PlayerEntryService.h"
#include "RepComponent.h"
#include "ServerFrameEventDispatcher.h"
#include "ServerWorldBootstrap.h"
#include "ServerWorldTransferBinding.h"
#include "ServerWorldTransferCommitter.h"
#include "SessionBindingRegistry.h"
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
		NetworkRuntime& network,
		SessionBindingRegistry& sessionBindings,
		PlayerEntryService& playerEntryService,
		double nowSec)
	{
		return ServerFrameEventDispatcher::Dispatch(
			frameResult,
			framework,
			network,
			sessionBindings,
			playerEntryService,
			nowSec);
	}

	bool RunAppLikeFrame(
		FrameworkRuntime& framework,
		NetworkRuntime& network,
		SessionBindingRegistry& sessionBindings,
		PlayerEntryService& playerEntryService,
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

		if (!ServerWorldTransferCommitter::Commit(framework, sessionBindings))
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
			network,
			sessionBindings,
			playerEntryService,
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
	SessionBindingRegistry sessionBindings;
	ServerWorldTransferBinding transferBinding(framework, sessionBindings);
	ServerWorldBootstrapFactory bootstrapFactory;
	ServerWorldBootstrapDefinitionProvider bootstrapDefinitions;
	WorldId startupWorldId = WorldId::Invalid();

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

	NetworkRuntime network(NetworkRuntime::Config{
		.workerThreadCount = 1,
		.listenPort = 7011,
		.maxSessions = 8
	});
	if (!network.Initialize())
	{
		std::cout << "[WorldTransitionSmoke] network initialize failed.\n";
		framework.Shutdown();
		return false;
	}

	PlayerEntryService playerEntryService(PlayerEntryService::Dependencies{
		.framework = &framework,
		.startupWorldId = &startupWorldId
	});

	if (!playerEntryService.BeginAuthenticatedEntry(kSessionId))
	{
		std::cout << "[WorldTransitionSmoke] entry begin failed.\n";
		network.Shutdown();
		framework.Shutdown();
		return false;
	}

	const PlayerEntryResult entryResult =
		playerEntryService.RequestCharacterSelect(kSessionId, CharacterId::Knight);
	if (!entryResult.Succeeded())
	{
		std::cout << "[WorldTransitionSmoke] character select failed."
			<< " code=" << static_cast<int>(entryResult.code)
			<< "\n";
		network.Shutdown();
		framework.Shutdown();
		return false;
	}

	double nowSec = 0.0;
	uint64_t frameIndex = 1;
	FrameworkRuntime::FrameResult frameResult{};

	network.BeginSendStage();
	nowSec += kDtSec;
	if (!RunAppLikeFrame(
		framework,
		network,
		sessionBindings,
		playerEntryService,
		frameIndex++,
		nowSec,
		kDtSec,
		frameResult))
	{
		network.Shutdown();
		framework.Shutdown();
		return false;
	}

	const SessionBinding* loginBinding =
		sessionBindings.FindBySession(kSessionId);
	if (loginBinding == nullptr ||
		loginBinding->currentWorldId != startupWorldId ||
		!loginBinding->controlledNetId.IsValid())
	{
		std::cout << "[WorldTransitionSmoke] login binding was not established.\n";
		network.Shutdown();
		framework.Shutdown();
		return false;
	}

	const NetId playerNetId = loginBinding->controlledNetId;
	const NetBindingLocation sourceLocation =
		framework.FindNetBinding(playerNetId);
	if (!sourceLocation.IsValid() || sourceLocation.worldId != startupWorldId)
	{
		std::cout << "[WorldTransitionSmoke] source net binding invalid after login.\n";
		network.Shutdown();
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
		network.Shutdown();
		framework.Shutdown();
		return false;
	}

	for (uint32_t step = 0; step < 12; ++step)
	{
		network.BeginSendStage();
		nowSec += kDtSec;
		if (!RunAppLikeFrame(
			framework,
			network,
			sessionBindings,
			playerEntryService,
			frameIndex++,
			nowSec,
			kDtSec,
			frameResult))
		{
			network.Shutdown();
			framework.Shutdown();
			return false;
		}
	}

	const SessionBinding* transferBindingResult =
		sessionBindings.FindBySession(kSessionId);
	if (transferBindingResult == nullptr ||
		transferBindingResult->controlledNetId != playerNetId ||
		transferBindingResult->currentWorldId == startupWorldId)
	{
		std::cout << "[WorldTransitionSmoke] session binding did not move to target.\n";
		network.Shutdown();
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
		network.Shutdown();
		framework.Shutdown();
		return false;
	}

	const WorldInstanceRecord* sourceRecord =
		framework.FindWorldRecord(startupWorldId);
	if (sourceRecord == nullptr || sourceRecord->activePlayers != 0)
	{
		std::cout << "[WorldTransitionSmoke] source active player count invalid.\n";
		network.Shutdown();
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
		network.Shutdown();
		framework.Shutdown();
		return false;
	}

	WorldInstance* targetWorld =
		framework.FindWorld(transferBindingResult->currentWorldId);
	if (targetWorld == nullptr ||
		!targetWorld->GetRuntime().MakeView().IsAlive(targetLocation.entity))
	{
		std::cout << "[WorldTransitionSmoke] target entity is not alive.\n";
		network.Shutdown();
		framework.Shutdown();
		return false;
	}

	std::cout << "[WorldTransitionSmoke] Passed."
		<< " transferId=" << transferId
		<< " sourceWorldId=" << startupWorldId.GetRaw()
		<< " targetWorldId=" << transferBindingResult->currentWorldId.GetRaw()
		<< " netId=" << playerNetId.GetRaw()
		<< "\n";

	network.Shutdown();
	framework.Shutdown();
	return true;
}
