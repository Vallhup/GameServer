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