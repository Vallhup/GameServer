#include "pch.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

#include "AsyncIOTypes.h"
#include "DynamicTaskScheduler.h"
#include "DynamicTaskTypes.h"
#include "ExecutionContextTypes.h"
#include "ExecutionCoreTypes.h"
#include "ExecutionGraphTypes.h"
#include "ExecutionRuntimeTypes.h"
#include "ExecutionSourceTypes.h"
#include "IExecutorIOSink.h"
#include "INetworkBackend.h"
#include "TaskExecutor.h"

// ============================================================================
//  AsyncIO SmokeTest
//
//  검증 항목:
//   1. IsTerminalNodeState(Suspended) == false
//   2. DynamicTaskScheduler 멀티스레드 Submit → DrainInto 전량 수집
//   3. 같은 프레임 내 Suspend → PushCompletion → Resume → Succeed
//   4. Suspend 시 remainingSimulateNodes 미감소 (행동 증명)
//   5. 완료 미도착 + deadline 초과 → Suspended → Canceled
// ============================================================================

namespace
{
    // -------------------------------------------------------------------------
    // 공통 유틸리티
    // -------------------------------------------------------------------------

    static void LogTestBanner(const char* name)
    {
        std::cout << "\n[ASYNCIO_TEST] " << name << "\n";
    }

    // -------------------------------------------------------------------------
    // MockNetworkBackend
    //
    // WaitForWork: 조건 변수로 500µs 대기 (또는 WakeWorker 호출 시 즉시 복귀).
    // WakeWorker:  cv.notify_one() + 호출 횟수 카운트.
    // -------------------------------------------------------------------------

    struct MockNetworkBackend final : INetworkBackend
    {
        std::mutex                    cvMtx;
        std::condition_variable       cv;
        std::atomic<uint32_t>         wakeCount{ 0 };

        bool WaitForWork(uint32_t /*workerIdx*/,
                         std::chrono::microseconds timeout) noexcept override
        {
            std::unique_lock lock{ cvMtx };
            cv.wait_for(lock, timeout);
            return false;
        }

        void WakeWorker() noexcept override
        {
            wakeCount.fetch_add(1, std::memory_order_relaxed);
            cv.notify_one();
        }

        bool Send(SessionId /*sessionId*/,
                  std::span<const uint8_t> /*payload*/) noexcept override
        {
            return true;
        }

        void FlushSend() noexcept override {}

        void Disconnect(SessionId /*sessionId*/) noexcept override {}

        uint32_t GetSessionCount() const noexcept override { return 0; }
    };

    // -------------------------------------------------------------------------
    // TestRig — TaskExecutor_SmokeTest.cpp와 동일한 경량 그래프 설정 도우미
    // -------------------------------------------------------------------------

    static ExecutionOps MakeDummyOps()
    {
        return ExecutionOps(
            reinterpret_cast<WorldManager*>(0x1),
            reinterpret_cast<WorldRegistry*>(0x1),
            reinterpret_cast<WorldTransferService*>(0x1),
            reinterpret_cast<WorldAdmissionService*>(0x1),
            reinterpret_cast<NetIdRegistry*>(0x1),
            reinterpret_cast<PresenceManager*>(0x1));
    }

    struct TestRig
    {
        FrameTaskGraph                        graph{};
        ExecutionSourceRegistry               sources{};
        ExecutionOps                          ops{ MakeDummyOps() };
        std::vector<WorldRuntime*>            runtimeByScopeBacking{};
        std::unique_ptr<ExecNodeRuntime[]>    nodeBacking{};
        std::unique_ptr<ExecScopeRuntime[]>   scopeBacking{};
        FrameExecContext                      frame{};
        ExecRuntimeState                      runtime{};

        void Finalize()
        {
            runtimeByScopeBacking.assign(graph.scopeCount, nullptr);

            nodeBacking = graph.nodes.empty()
                ? nullptr
                : std::make_unique<ExecNodeRuntime[]>(graph.nodes.size());

            scopeBacking = (graph.scopeCount > 0)
                ? std::make_unique<ExecScopeRuntime[]>(graph.scopeCount)
                : nullptr;

            frame.graph = &graph;
            frame.ops   = &ops;
            frame.runtimeByScope = std::span<WorldRuntime*>(
                runtimeByScopeBacking.data(),
                runtimeByScopeBacking.size());

            runtime.nodes = std::span<ExecNodeRuntime>(
                nodeBacking.get(),
                graph.nodes.size());
            runtime.scopes = std::span<ExecScopeRuntime>(
                scopeBacking.get(),
                graph.scopeCount);
        }
    };

    static void AddSource(
        ExecutionSourceRegistry& registry,
        ExecToken token,
        ExecPhase phase,
        ExecLane lane,
        ExecNodeKind kind,
        ExecFn fn,
        uint32_t flags = ExecNodeFlag_None,
        const char* debugName = "")
    {
        ExecutionSourceDesc desc{};
        desc.token     = token;
        desc.phase     = phase;
        desc.lane      = lane;
        desc.kind      = kind;
        desc.flags     = flags;
        desc.fn        = fn;
        desc.debugName = debugName;
        const bool ok  = registry.Register(desc);
        assert(ok);
    }

    static ExecNodeRecord MakeNode(
        ExecNodeId id, ExecScopeId scopeId,
        ExecPhase phase, ExecLane lane, ExecNodeKind kind,
        ExecToken token,
        uint32_t predBegin, uint32_t predCount,
        uint32_t succBegin, uint32_t succCount,
        uint32_t flags = ExecNodeFlag_None)
    {
        ExecNodeRecord n{};
        n.id          = id;
        n.scopeId     = scopeId;
        n.phase       = phase;
        n.lane        = lane;
        n.kind        = kind;
        n.flags       = flags;
        n.predBegin   = predBegin;
        n.predCount   = predCount;
        n.succBegin   = succBegin;
        n.succCount   = succCount;
        n.sourceToken = token;
        return n;
    }

    static void ExpectNodeState(
        const ExecRuntimeState& runtime,
        ExecNodeId nodeId,
        ExecNodeState expected)
    {
        const ExecNodeRuntime* rt = runtime.TryGetNode(nodeId);
        assert(rt != nullptr);
        const ExecNodeState actual = rt->state.load(std::memory_order_acquire);
        if (actual != expected)
        {
            std::cout
                << "[FAIL] node=" << nodeId
                << " expected=" << static_cast<int>(expected)
                << " actual="   << static_cast<int>(actual)
                << "\n";
            assert(false);
        }
    }

    // -------------------------------------------------------------------------
    // ExecFn 환경 — 스레드 안전
    // -------------------------------------------------------------------------

    struct AsyncIOExecEnv
    {
        std::atomic<IoHandle*> capturedHandle{ nullptr };
        std::atomic<bool>      suspended{ false };    // 첫 번째 호출에서 Suspend 반환 후 set
        std::atomic<bool>      resumed{ false };       // 두 번째 호출(재개) 시 set
        std::atomic<int>       executeCount{ 0 };      // 총 ExecFn 호출 횟수
    };

    static AsyncIOExecEnv* g_asyncEnv = nullptr;

    // 첫 호출: RequestSuspend() + Suspend 반환
    // 두 번째 호출(재개): resumed 플래그 설정 + Success 반환
    static ExecCallResult SuspendOnFirstCall(NodeExecContext& ctx)
    {
        assert(g_asyncEnv != nullptr);
        assert(ctx.IsValid());

        const int callIndex = g_asyncEnv->executeCount.fetch_add(1, std::memory_order_acq_rel);

        if (callIndex == 0)
        {
            IoHandle* handle = ctx.RequestSuspend();
            assert(handle != nullptr && "IoHandle pool exhausted or ioProvider not set");
            g_asyncEnv->capturedHandle.store(handle, std::memory_order_release);
            g_asyncEnv->suspended.store(true, std::memory_order_release);
            return ExecCallResult::Suspend;
        }

        // 재개 경로
        g_asyncEnv->resumed.store(true, std::memory_order_release);
        return ExecCallResult::Success;
    }

    // Suspend만 반환 — 재개되지 않도록 설계 (sweep 테스트용)
    static ExecCallResult SuspendAndNeverResume(NodeExecContext& ctx)
    {
        assert(g_asyncEnv != nullptr);
        assert(ctx.IsValid());

        const int callIndex = g_asyncEnv->executeCount.fetch_add(1, std::memory_order_acq_rel);
        assert(callIndex == 0 && "SuspendAndNeverResume: unexpected re-invocation");

        IoHandle* handle = ctx.RequestSuspend();
        assert(handle != nullptr);
        g_asyncEnv->capturedHandle.store(handle, std::memory_order_release);
        g_asyncEnv->suspended.store(true, std::memory_order_release);
        return ExecCallResult::Suspend;
    }

    // =========================================================================
    // Test 01 — IsTerminalNodeState 타입 검사
    //
    // Suspended는 터미널이 아닌 상태이므로 false를 반환해야 한다.
    // 실제 터미널 상태(Succeeded/Failed/Canceled/Skipped)는 true를 반환해야 한다.
    // =========================================================================

    static void Test_AsyncIO_01_IsTerminalNodeState_Suspended_IsFalse()
    {
        LogTestBanner(__FUNCTION__);

        // Suspended: 비터미널
        assert(!IsTerminalNodeState(ExecNodeState::Suspended));

        // 비터미널 상태 전체
        assert(!IsTerminalNodeState(ExecNodeState::NotReady));
        assert(!IsTerminalNodeState(ExecNodeState::Ready));
        assert(!IsTerminalNodeState(ExecNodeState::Queued));
        assert(!IsTerminalNodeState(ExecNodeState::Running));

        // 터미널 상태 전체
        assert(IsTerminalNodeState(ExecNodeState::Succeeded));
        assert(IsTerminalNodeState(ExecNodeState::Failed));
        assert(IsTerminalNodeState(ExecNodeState::Canceled));
        assert(IsTerminalNodeState(ExecNodeState::Skipped));
    }

    // =========================================================================
    // Test 02 — DynamicTaskScheduler 멀티스레드 Submit → DrainInto
    //
    // N개 스레드에서 동시에 Submit한 뒤, 프레임 경계(단일 스레드)에서 DrainInto를
    // 호출했을 때 전량 수집되는지 검증한다.
    // =========================================================================

    static void Test_AsyncIO_02_DynamicTaskScheduler_ConcurrentSubmit_DrainAll()
    {
        LogTestBanner(__FUNCTION__);

        constexpr int kThreadCount   = 8;
        constexpr int kPerThread     = 500;
        constexpr int kExpectedTotal = kThreadCount * kPerThread;

        DynamicTaskScheduler scheduler;

        std::vector<std::thread> threads;
        threads.reserve(kThreadCount);

        for (int t = 0; t < kThreadCount; ++t)
        {
            threads.emplace_back([&, t]()
            {
                for (int i = 0; i < kPerThread; ++i)
                {
                    DynamicTaskRequest req{};
                    req.typeId           = static_cast<DynamicTaskTypeId>(t * kPerThread + i + 1);
                    req.scopeId          = static_cast<ExecScopeId>(t % 4);
                    req.payloadKey       = static_cast<uint64_t>(t * 10000 + i);
                    req.requestFrameIndex = static_cast<uint64_t>(i);
                    scheduler.Submit(req);
                }
            });
        }

        for (auto& thr : threads)
            thr.join();

        std::vector<DynamicTaskRequest> collected;
        collected.reserve(kExpectedTotal);
        scheduler.DrainInto(collected);

        std::cout
            << "[TRACE] DynamicTaskScheduler drained " << collected.size()
            << " / " << kExpectedTotal << " requests\n";

        assert(static_cast<int>(collected.size()) == kExpectedTotal);

        // 두 번째 DrainInto는 빈 상태여야 한다.
        std::vector<DynamicTaskRequest> second;
        scheduler.DrainInto(second);
        assert(second.empty());
    }

    // =========================================================================
    // Test 03 — 같은 프레임 Suspend → PushCompletion → Resume → Succeed
    //
    // 노드가 첫 실행에서 Suspend를 반환하면, 별도 스레드에서 PushCompletion을
    // 호출했을 때 같은 프레임 내에서 재개되어 Succeeded 상태로 완료되어야 한다.
    // =========================================================================

    static void Test_AsyncIO_03_SuspendResume_SameFrame_NodeSucceeds()
    {
        LogTestBanner(__FUNCTION__);

        AsyncIOExecEnv env{};
        g_asyncEnv = &env;

        constexpr ExecToken kSuspendToken = 1;

        TestRig rig{};
        rig.graph.scopeCount      = 1;
        rig.graph.scopeToWorld    = { 3001 };
        rig.graph.simulateNodeCount = 1;

        AddSource(
            rig.sources,
            kSuspendToken,
            ExecPhase::Simulate,
            ExecLane::Parallel,
            ExecNodeKind::StaticSystem,
            &SuspendOnFirstCall,
            ExecNodeFlag_None,
            "SuspendOnFirstCall");

        rig.graph.nodes.push_back(
            MakeNode(0, 0,
                ExecPhase::Simulate, ExecLane::Parallel, ExecNodeKind::StaticSystem,
                kSuspendToken,
                0, 0, 0, 0));

        rig.Finalize();

        TaskExecutor executor(2);
        IExecutorIOSink& sink = executor;

        // 별도 스레드: 노드가 Suspended가 될 때까지 기다린 뒤 완료 신호를 전달한다.
        std::thread pusher([&]()
        {
            while (!env.suspended.load(std::memory_order_acquire))
                std::this_thread::sleep_for(std::chrono::microseconds(10));

            IoHandle* handle = env.capturedHandle.load(std::memory_order_acquire);
            assert(handle != nullptr);

            IoResult result{ true, nullptr, 0 };
            sink.PushCompletion(CompletionEntry{ handle, result });

            std::cout << "[TRACE] Test_03: PushCompletion called\n";
        });

        const bool ok = executor.ExecuteFrame(rig.frame, rig.runtime, rig.sources);
        pusher.join();

        std::cout
            << "[TRACE] Test_03:"
            << " suspended=" << env.suspended.load()
            << " resumed="   << env.resumed.load()
            << " executeCount=" << env.executeCount.load()
            << "\n";

        assert(ok);
        assert(env.suspended.load());
        assert(env.resumed.load());
        assert(env.executeCount.load() == 2);   // Suspend + Resume 각 1회
        ExpectNodeState(rig.runtime, 0, ExecNodeState::Succeeded);
        assert(rig.runtime.signals.simulatePhaseDone.load(std::memory_order_acquire));

        g_asyncEnv = nullptr;
    }

    // =========================================================================
    // Test 04 — Suspend 시 remainingSimulateNodes 미감소 행동 증명
    //
    // Suspend 반환 시 remainingSimulateNodes가 감소하지 않음을 행동으로 증명한다:
    // 2개 Simulate 노드(A: 즉시 성공, B: Suspend 후 재개) 환경에서,
    // A가 완료돼도 ExecuteFrame이 B의 재개를 기다려야 한다.
    // B가 재개된 후에야 simulatePhaseDone이 설정되어야 한다.
    // =========================================================================

    static AsyncIOExecEnv* g_normEnv = nullptr;

    static ExecCallResult ImmediateSuccess(NodeExecContext& ctx)
    {
        assert(ctx.IsValid());
        return ExecCallResult::Success;
    }

    static ExecCallResult SuspendOnFirstCallB(NodeExecContext& ctx)
    {
        assert(g_asyncEnv != nullptr);
        assert(ctx.IsValid());

        const int callIndex = g_asyncEnv->executeCount.fetch_add(1, std::memory_order_acq_rel);

        if (callIndex == 0)
        {
            IoHandle* handle = ctx.RequestSuspend();
            assert(handle != nullptr);
            g_asyncEnv->capturedHandle.store(handle, std::memory_order_release);
            g_asyncEnv->suspended.store(true, std::memory_order_release);
            return ExecCallResult::Suspend;
        }

        g_asyncEnv->resumed.store(true, std::memory_order_release);
        return ExecCallResult::Success;
    }

    static void Test_AsyncIO_04_Suspend_RemainingSimulateNodes_NotDecremented()
    {
        LogTestBanner(__FUNCTION__);

        AsyncIOExecEnv env{};
        g_asyncEnv = &env;

        constexpr ExecToken kTokenA = 1;
        constexpr ExecToken kTokenB = 2;

        TestRig rig{};
        rig.graph.scopeCount        = 1;
        rig.graph.scopeToWorld      = { 4001 };
        rig.graph.simulateNodeCount = 2;

        AddSource(
            rig.sources, kTokenA,
            ExecPhase::Simulate, ExecLane::Parallel, ExecNodeKind::StaticSystem,
            &ImmediateSuccess, ExecNodeFlag_None, "ImmediateSuccess");
        AddSource(
            rig.sources, kTokenB,
            ExecPhase::Simulate, ExecLane::Parallel, ExecNodeKind::StaticSystem,
            &SuspendOnFirstCallB, ExecNodeFlag_None, "SuspendOnFirstCallB");

        rig.graph.nodes.push_back(
            MakeNode(0, 0,
                ExecPhase::Simulate, ExecLane::Parallel, ExecNodeKind::StaticSystem,
                kTokenA, 0, 0, 0, 0));
        rig.graph.nodes.push_back(
            MakeNode(1, 0,
                ExecPhase::Simulate, ExecLane::Parallel, ExecNodeKind::StaticSystem,
                kTokenB, 0, 0, 0, 0));

        rig.Finalize();

        TaskExecutor executor(2);
        IExecutorIOSink& sink = executor;

        // 2ms 지연 후 완료 신호 — A가 즉시 완료되더라도 B 재개까지 프레임이 종료되면 안 된다.
        std::thread pusher([&]()
        {
            while (!env.suspended.load(std::memory_order_acquire))
                std::this_thread::sleep_for(std::chrono::microseconds(10));

            std::this_thread::sleep_for(std::chrono::milliseconds(2));

            IoHandle* handle = env.capturedHandle.load(std::memory_order_acquire);
            assert(handle != nullptr);

            IoResult result{ true, nullptr, 0 };
            sink.PushCompletion(CompletionEntry{ handle, result });

            std::cout << "[TRACE] Test_04: PushCompletion called (2ms delayed)\n";
        });

        const bool ok = executor.ExecuteFrame(rig.frame, rig.runtime, rig.sources);
        pusher.join();

        std::cout
            << "[TRACE] Test_04:"
            << " suspended=" << env.suspended.load()
            << " resumed="   << env.resumed.load()
            << "\n";

        // B가 재개되지 않은 상태에서 ExecuteFrame이 일찍 반환됐다면
        // remainingSimulateNodes가 Suspend 시 잘못 감소된 것이다.
        assert(ok);
        assert(env.resumed.load() && "B 미재개 = remainingSimulateNodes가 Suspend 시 잘못 감소됨");
        ExpectNodeState(rig.runtime, 0, ExecNodeState::Succeeded);
        ExpectNodeState(rig.runtime, 1, ExecNodeState::Succeeded);
        assert(rig.runtime.signals.simulatePhaseDone.load(std::memory_order_acquire));

        g_asyncEnv = nullptr;
    }

    // =========================================================================
    // Test 05 — 완료 미도착 + deadline 초과 → Suspended → Canceled
    //
    // 노드가 Suspend 반환 후 완료 신호가 오지 않으면, simulate deadline 초과 시
    // TriggerSweep이 Suspended → Canceled로 전환해야 한다.
    //
    // 프레임 예산(~11ms) 이내에 완료가 없으므로 테스트 시간이 약 11ms 소요된다.
    // =========================================================================

    static void Test_AsyncIO_05_NoCompletion_DeadlineExceeded_NodeCanceled()
    {
        LogTestBanner(__FUNCTION__);

        AsyncIOExecEnv env{};
        g_asyncEnv = &env;

        constexpr ExecToken kToken = 1;

        TestRig rig{};
        rig.graph.scopeCount        = 1;
        rig.graph.scopeToWorld      = { 5001 };
        rig.graph.simulateNodeCount = 1;

        AddSource(
            rig.sources, kToken,
            ExecPhase::Simulate, ExecLane::Parallel, ExecNodeKind::StaticSystem,
            &SuspendAndNeverResume, ExecNodeFlag_None, "SuspendAndNeverResume");

        rig.graph.nodes.push_back(
            MakeNode(0, 0,
                ExecPhase::Simulate, ExecLane::Parallel, ExecNodeKind::StaticSystem,
                kToken, 0, 0, 0, 0));

        rig.Finalize();

        const auto testStart = std::chrono::steady_clock::now();

        TaskExecutor executor(2);
        const bool ok = executor.ExecuteFrame(rig.frame, rig.runtime, rig.sources);

        const auto elapsed = std::chrono::steady_clock::now() - testStart;
        const auto elapsedMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

        std::cout
            << "[TRACE] Test_05:"
            << " suspended=" << env.suspended.load()
            << " executeCount=" << env.executeCount.load()
            << " elapsedMs=" << elapsedMs
            << "\n";

        // deadline 초과 sweep → Canceled, ExecuteFrame은 성공 반환
        assert(ok);
        assert(env.suspended.load());
        assert(env.executeCount.load() == 1); // 재개 없음 — Sweep이 Canceled 처리
        ExpectNodeState(rig.runtime, 0, ExecNodeState::Canceled);
        assert(rig.runtime.signals.simulatePhaseDone.load(std::memory_order_acquire));

        g_asyncEnv = nullptr;
    }

} // namespace

// ============================================================================
// 진입점
// ============================================================================

void RunAsyncIOSmokeTests()
{
    Test_AsyncIO_01_IsTerminalNodeState_Suspended_IsFalse();
    std::cout << "[PASS] Test_AsyncIO_01_IsTerminalNodeState_Suspended_IsFalse\n";

    Test_AsyncIO_02_DynamicTaskScheduler_ConcurrentSubmit_DrainAll();
    std::cout << "[PASS] Test_AsyncIO_02_DynamicTaskScheduler_ConcurrentSubmit_DrainAll\n";

    Test_AsyncIO_03_SuspendResume_SameFrame_NodeSucceeds();
    std::cout << "[PASS] Test_AsyncIO_03_SuspendResume_SameFrame_NodeSucceeds\n";

    Test_AsyncIO_04_Suspend_RemainingSimulateNodes_NotDecremented();
    std::cout << "[PASS] Test_AsyncIO_04_Suspend_RemainingSimulateNodes_NotDecremented\n";

    Test_AsyncIO_05_NoCompletion_DeadlineExceeded_NodeCanceled();
    std::cout << "[PASS] Test_AsyncIO_05_NoCompletion_DeadlineExceeded_NodeCanceled\n";

    std::cout << "\nAll AsyncIO smoke tests passed.\n";
}
