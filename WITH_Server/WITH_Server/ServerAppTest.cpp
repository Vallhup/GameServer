#include "pch.h"
#include "ServerAppTest.h"
#include "ServerApp.h"

#include <chrono>
#include <iostream>
#include <thread>

#include "FrameworkRuntime.h"
#include "RepComponent.h"
#include "ServerWorldBootstrap.h"
#include "SessionBindingRegistry.h"
#include "WorldInstance.h"

namespace
{
	using namespace std::chrono_literals;

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

	const Entity reservedEntity = runtime.ReserveEntity();
	if (reservedEntity.IsNull())
	{
		std::cout << "[ServerAppTest] ReserveEntity failed.\n";
		framework.Shutdown();
		return false;
	}

	runtime.DeferredAddComponent<ReplicatedTag>(reservedEntity);

	SpawnTypeComp typeComp =
	{
		.entityType = EntityType::Character,
		.faction = Faction::Player,
		.charType = CharacterType::Knight
	};
	runtime.DeferredUpsertComponent<SpawnTypeComp>(
		reservedEntity, typeComp);

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
		spawnEvent.entity != reservedEntity ||
		!spawnEvent.netId.IsValid())
	{
		std::cout << "[ServerAppTest] Spawn event contents are invalid.\n";
		framework.Shutdown();
		return false;
	}

	if (framework.FindNetId(startupWorldId, reservedEntity) != spawnEvent.netId)
	{
		std::cout << "[ServerAppTest] Framework NetId lookup mismatch after spawn.\n";
		framework.Shutdown();
		return false;
	}

	SessionBindingRegistry sessionBindings;
	constexpr SessionId testSessionId = 1001;
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

	runtime.DeferredDestroyEntity(reservedEntity);

	FrameworkRuntime::FrameResult despawnFrame{};
	if (!RunSingleAppLikeFrame(
		framework,
		4,
		4.0 / 30.0,
		1.0 / 30.0,
		despawnFrame))
	{
		std::cout << "[ServerAppTest] Despawn frame failed.\n";
		PrintRuntimeState("Despawn", runtime);
		framework.Shutdown();
		return false;
	}

	if (despawnFrame.events.despawns.size() != 1)
	{
		std::cout << "[ServerAppTest] Expected one despawn event, got "
			<< despawnFrame.events.despawns.size() << ".\n";
		std::cout << "[ServerAppTest] Despawn frame selectedWorldCount="
			<< despawnFrame.selectedWorldCount << ".\n";
		PrintRuntimeState("DespawnNoEvent", runtime);
		framework.Shutdown();
		return false;
	}

	const auto& despawnEvent = despawnFrame.events.despawns.front();
	if (despawnEvent.worldId != startupWorldId ||
		despawnEvent.entity != reservedEntity ||
		despawnEvent.netId != spawnEvent.netId)
	{
		std::cout << "[ServerAppTest] Despawn event contents are invalid.\n";
		framework.Shutdown();
		return false;
	}

	if (framework.FindNetId(startupWorldId, reservedEntity).IsValid() ||
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
