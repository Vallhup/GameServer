#include "pch.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <span>

#include "TaskExecutor.h"
#include "ExecutionContextTypes.h"
#include "ExecutionCoreTypes.h"
#include "ExecutionGraphTypes.h"
#include "ExecutionRuntimeTypes.h"
#include "ExecutionSourceTypes.h"
#include "ExecutionGraphBuilder.h"
#include "ExecutionGraphBuildPolicy.h"
#include "ExecutionOps.h"
#include "IWorldDefinitionProvider.h"
#include "IWorldInstanceFactory.h"
#include "WorldRuntime.h"
#include "WorldDef.h"
#include "WorldDefinitionBootstrap.h"
#include "WorldExecutionModelTypes.h"
#include "WorldManager.h"
#include "WorldRegistry.h"
#include "WorldScheduler.h"
#include "WorldTransferProfileRegistry.h"

namespace
{
    struct TestRecorder
    {
        std::mutex mtx;
        std::vector<std::string> events;

        void Push(const std::string& s)
        {
            std::lock_guard lock{ mtx };
            events.push_back(s);
        }

        bool Equals(const std::vector<std::string>& expected) const
        {
            return events == expected;
        }

        size_t Count(const std::string& s) const
        {
            return static_cast<size_t>(
                std::count(events.begin(), events.end(), s));
        }
    };

    struct CallbackEnv
    {
        TestRecorder recorder;
        bool failA{ false };
        bool failB{ false };
        bool failC{ false };
    };

    struct FaultProbeComponent final : Component
    {
        int value{ 0 };
    };

    class NoopWorldInstanceImpl final : public IWorldInstanceImpl
    {
    public:
        bool OnCreate(WorldRuntime& runtime) override
        {
            runtime.RegisterStorage<FaultProbeComponent>();
            return true;
        }

        bool OnStart(WorldRuntime& runtime) override
        {
            (void)runtime;
            return true;
        }

        void OnStop(WorldRuntime& runtime) override
        {
            (void)runtime;
        }
    };

    class TestWorldInstanceFactory final : public IWorldInstanceFactory
    {
    public:
        std::unique_ptr<IWorldInstanceImpl> Create(
            const WorldDef& def,
            WorldId worldId) override
        {
            (void)def;
            (void)worldId;
            return std::make_unique<NoopWorldInstanceImpl>();
        }
    };

    struct StaticDefinitionProvider final : IWorldDefinitionProvider
    {
        std::vector<ExecutionSourceDesc> sources;
        std::vector<WorldExecutionModel> models;
        std::vector<WorldDef> defs;

        bool RegisterExecutionSources(
            ExecutionSourceRegistry& sourceRegistry) const override
        {
            for (const ExecutionSourceDesc& source : sources)
            {
                if (!sourceRegistry.Register(source))
                    return false;
            }

            return true;
        }

        bool RegisterExecutionModels(
            const ExecutionSourceRegistry& sourceRegistry,
            WorldExecutionModelRegistry& executionModelRegistry) const override
        {
            for (const WorldExecutionModel& model : models)
            {
                if (!executionModelRegistry.Register(model, sourceRegistry))
                    return false;
            }

            return true;
        }

        bool RegisterWorldDefs(WorldRegistry& worldRegistry) const override
        {
            for (const WorldDef& def : defs)
            {
                if (!worldRegistry.RegisterWorldDef(def))
                    return false;
            }

            return true;
        }
    };

    static CallbackEnv* g_env = nullptr;

    static const char* ToString(ExecNodeState state) noexcept
    {
        switch (state)
        {
        case ExecNodeState::NotReady: return "NotReady";
        case ExecNodeState::Ready: return "Ready";
        case ExecNodeState::Queued: return "Queued";
        case ExecNodeState::Running: return "Running";
        case ExecNodeState::Succeeded: return "Succeeded";
        case ExecNodeState::Failed: return "Failed";
        case ExecNodeState::Canceled: return "Canceled";
        case ExecNodeState::Skipped: return "Skipped";
        default: return "UnknownNodeState";
        }
    }

    static const char* ToString(ExecScopePhase phase) noexcept
    {
        switch (phase)
        {
        case ExecScopePhase::Open: return "Open";
        case ExecScopePhase::CancelRequested: return "CancelRequested";
        case ExecScopePhase::Draining: return "Draining";
        case ExecScopePhase::Closed: return "Closed";
        default: return "UnknownScopePhase";
        }
    }

    static const char* ToString(WorldRuntimeCommitState state) noexcept
    {
        switch (state)
        {
        case WorldRuntimeCommitState::NotCommitted: return "NotCommitted";
        case WorldRuntimeCommitState::CommitSucceeded: return "CommitSucceeded";
        case WorldRuntimeCommitState::CommitFailed: return "CommitFailed";
        default: return "UnknownCommitState";
        }
    }

    static const char* ToString(WorldRuntimeLifecycleFlushState state) noexcept
    {
        switch (state)
        {
        case WorldRuntimeLifecycleFlushState::NotFlushed: return "NotFlushed";
        case WorldRuntimeLifecycleFlushState::Flushed: return "Flushed";
        default: return "UnknownLifecycleFlushState";
        }
    }

    static void LogTestBanner(const char* testName)
    {
        std::cout << "\n[TEST] " << testName << "\n";
    }

    static void LogRecorderState(const TestRecorder& recorder, const char* label)
    {
        std::ostringstream oss;
        oss << "[TRACE] " << label << " events(" << recorder.events.size() << ")";
        for (const std::string& event : recorder.events)
            oss << ' ' << event;

        std::cout << oss.str() << "\n";
    }

    static void LogNodeState(
        const ExecRuntimeState& runtime,
        ExecNodeId nodeId,
        const char* label)
    {
        const ExecNodeRuntime* rt = runtime.TryGetNode(nodeId);
        assert(rt != nullptr);

        std::cout
            << "[TRACE] " << label
            << " node=" << nodeId
            << " state=" << ToString(rt->state.load())
            << " remainingDeps=" << rt->remainingDeps.load()
            << "\n";
    }

    static void LogScopeState(
        const ExecRuntimeState& runtime,
        ExecScopeId scopeId,
        const char* label)
    {
        const ExecScopeRuntime* rt = runtime.TryGetScope(scopeId);
        assert(rt != nullptr);

        std::cout
            << "[TRACE] " << label
            << " scope=" << scopeId
            << " phase=" << ToString(rt->phase.load())
            << " flags=" << static_cast<uint32_t>(rt->flags.load())
            << " remainingNodes=" << rt->remainingNodes.load()
            << " closeCandidate=" << (rt->closeCandidate.load() ? "true" : "false")
            << "\n";
    }

    static void LogWorldRuntimeState(
        const WorldRuntime& runtime,
        const char* label)
    {
        const ECSView view = runtime.MakeView();

        std::cout
            << "[TRACE] " << label
            << " frame=" << runtime.FrameIndex()
            << " commit=" << ToString(runtime.GetCommitState())
            << " lifecycleFlush=" << ToString(runtime.GetLifecycleFlushState())
            << " fault=" << (runtime.IsFaulted() ? "true" : "false")
            << " outbox=" << runtime.LifecycleOutbox().size()
            << " aliveEntities=" << view.AliveEntities().size()
            << "\n";
    }

    static void LogSchedulerResult(
        const WorldSchedulerFrameResult& result,
        const BuildResult& buildResult,
        const char* label)
    {
        std::cout
            << "[TRACE] " << label
            << " success=" << (result.success ? "true" : "false")
            << " graphBuilt=" << (result.graphBuilt ? "true" : "false")
            << " executed=" << (result.executed ? "true" : "false")
            << " selectedWorldCount=" << result.selectedWorldCount
            << " failureReason=" << static_cast<uint32_t>(result.failureReason)
            << " diagnostics=" << buildResult.diagnostics.size()
            << "\n";
    }

    static ExecCallResult SimA(NodeExecContext& ctx)
    {
        assert(g_env != nullptr);
        assert(ctx.IsValid());

        g_env->recorder.Push("SimA");
        return g_env->failA ? ExecCallResult::Failed : ExecCallResult::Success;
    }

    static ExecCallResult SimB(NodeExecContext& ctx)
    {
        assert(g_env != nullptr);
        assert(ctx.IsValid());

        g_env->recorder.Push("SimB");
        return g_env->failB ? ExecCallResult::Failed : ExecCallResult::Success;
    }

    static ExecCallResult SimC(NodeExecContext& ctx)
    {
        assert(g_env != nullptr);
        assert(ctx.IsValid());

        g_env->recorder.Push("SimC");
        return g_env->failC ? ExecCallResult::Failed : ExecCallResult::Success;
    }

    static ExecCallResult CommitA(NodeExecContext& ctx)
    {
        assert(g_env != nullptr);
        assert(ctx.IsValid());

        g_env->recorder.Push("CommitA");
        return ExecCallResult::Success;
    }

    static ExecCallResult LifecycleA(NodeExecContext& ctx)
    {
        assert(g_env != nullptr);
        assert(ctx.IsValid());

        g_env->recorder.Push("LifecycleA");
        return ExecCallResult::Success;
    }

    static ExecCallResult ReconcileA(NodeExecContext& ctx)
    {
        assert(g_env != nullptr);
        assert(ctx.IsValid());

        g_env->recorder.Push("ReconcileA");
        return ExecCallResult::Success;
    }

    static ExecCallResult CommitViaOps(NodeExecContext& ctx)
    {
        assert(ctx.IsValid());
        ExecutionOps* ops = ctx.TryGetOps();
        assert(ops != nullptr);

        ops->CommitScope(ctx.scopeId, ctx.frame->runtimeByScope);
        return ExecCallResult::Success;
    }

    static ExecCallResult LifecycleViaOps(NodeExecContext& ctx)
    {
        assert(ctx.IsValid());
        ExecutionOps* ops = ctx.TryGetOps();
        assert(ops != nullptr);

        ops->FlushLifecycle(ctx.scopeId, ctx.frame->runtimeByScope);
        return ExecCallResult::Success;
    }

    static ExecCallResult ReconcileViaOps(NodeExecContext& ctx)
    {
        assert(ctx.IsValid());
        ExecutionOps* ops = ctx.TryGetOps();
        assert(ops != nullptr);

        ops->ReconcileScope(ctx.scopeId, ctx.frame->runtimeByScope);
        return ExecCallResult::Success;
    }

    static ExecutionOps MakeDummyValidOps()
    {
        // ?뚯뒪??callback?먯꽌??ops瑜??ㅼ젣濡??ъ슜?섏? ?딅뒗?ㅻ뒗 ?꾩젣???붾???
        return ExecutionOps(
            reinterpret_cast<WorldManager*>(0x1),
            reinterpret_cast<WorldRegistry*>(0x1),
            reinterpret_cast<WorldTransferService*>(0x1),
            reinterpret_cast<WorldAdmissionService*>(0x1),
            reinterpret_cast<PresenceManager*>(0x1)
        );
    }

    static ExecNodeRecord MakeNode(
        ExecNodeId id,
        ExecScopeId scopeId,
        ExecPhase phase,
        ExecLane lane,
        ExecNodeKind kind,
        ExecToken token,
        uint32_t predBegin,
        uint32_t predCount,
        uint32_t succBegin,
        uint32_t succCount,
        uint32_t flags = ExecNodeFlag_None)
    {
        ExecNodeRecord n{};
        n.id = id;
        n.scopeId = scopeId;
        n.phase = phase;
        n.lane = lane;
        n.kind = kind;
        n.flags = flags;
        n.predBegin = predBegin;
        n.predCount = predCount;
        n.succBegin = succBegin;
        n.succCount = succCount;
        n.sourceToken = token;
        return n;
    }

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
        desc.token = token;
        desc.phase = phase;
        desc.lane = lane;
        desc.kind = kind;
        desc.flags = flags;
        desc.fn = fn;
        desc.debugName = debugName;

        const bool ok = registry.Register(desc);
        assert(ok);
    }

    static ExecutionSourceDesc MakeSourceDesc(
        ExecToken token,
        ExecPhase phase,
        ExecLane lane,
        ExecNodeKind kind,
        ExecFn fn,
        uint32_t flags = ExecNodeFlag_None,
        const char* debugName = "")
    {
        ExecutionSourceDesc desc{};
        desc.token = token;
        desc.phase = phase;
        desc.lane = lane;
        desc.kind = kind;
        desc.flags = flags;
        desc.fn = fn;
        desc.debugName = debugName;
        return desc;
    }

    struct TestRig
    {
        FrameTaskGraph graph{};
        ExecutionSourceRegistry sources{};
        ExecutionOps ops{ MakeDummyValidOps() };

        std::vector<WorldRuntime*> runtimeByScopeBacking{};

        std::unique_ptr<ExecNodeRuntime[]> nodeBacking{};
        std::unique_ptr<ExecScopeRuntime[]> scopeBacking{};

        FrameExecContext frame{};
        ExecRuntimeState runtime{};

        void Finalize()
        {
            runtimeByScopeBacking.assign(graph.scopeCount, nullptr);

            if (!graph.nodes.empty())
                nodeBacking = std::make_unique<ExecNodeRuntime[]>(graph.nodes.size());
            else
                nodeBacking.reset();

            if (graph.scopeCount > 0)
                scopeBacking = std::make_unique<ExecScopeRuntime[]>(graph.scopeCount);
            else
                scopeBacking.reset();

            frame.graph = &graph;
            frame.ops = &ops;
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

    static void ExpectNodeState(
        const ExecRuntimeState& runtime,
        ExecNodeId nodeId,
        ExecNodeState expected)
    {
        const ExecNodeRuntime* rt = runtime.TryGetNode(nodeId);
        assert(rt != nullptr);
        assert(rt->state.load() == expected);
    }

    static void ExpectScopeClosed(
        const ExecRuntimeState& runtime,
        ExecScopeId scopeId)
    {
        const ExecScopeRuntime* rt = runtime.TryGetScope(scopeId);
        assert(rt != nullptr);
        assert(rt->phase.load() == ExecScopePhase::Closed);
    }

    static void ExpectScopeCanceled(
        const ExecRuntimeState& runtime,
        ExecScopeId scopeId)
    {
        const ExecScopeRuntime* rt = runtime.TryGetScope(scopeId);
        assert(rt != nullptr);

        const ExecScopePhase phase = rt->phase.load();
        assert(phase == ExecScopePhase::CancelRequested ||
            phase == ExecScopePhase::Closed);

        const uint8_t flags = rt->flags.load();
        assert(HasAnyScopeFlag(flags, ExecScopeFlag_WasCanceled));
    }

    static void Test_01_SingleSimulateSuccess()
    {
        LogTestBanner(__FUNCTION__);

        CallbackEnv env{};
        g_env = &env;

        TestRig rig{};
        rig.graph.scopeCount = 1;
        rig.graph.scopeToWorld = { 1001 };
        rig.graph.simulateNodeCount = 1;

        constexpr ExecToken kA = 1;
        AddSource(
            rig.sources,
            kA,
            ExecPhase::Simulate,
            ExecLane::Parallel,
            ExecNodeKind::StaticSystem,
            &SimA,
            ExecNodeFlag_None,
            "SimA");

        rig.graph.nodes.push_back(
            MakeNode(
                0, 0,
                ExecPhase::Simulate,
                ExecLane::Parallel,
                ExecNodeKind::StaticSystem,
                kA,
                0, 0,
                0, 0));

        rig.Finalize();

        TaskExecutor executor(2);
        const bool ok = executor.ExecuteFrame(rig.frame, rig.runtime, rig.sources);
        assert(ok);

        LogNodeState(rig.runtime, 0, "single-sim");
        LogScopeState(rig.runtime, 0, "single-sim");
        LogRecorderState(env.recorder, "single-sim");

        ExpectNodeState(rig.runtime, 0, ExecNodeState::Succeeded);
        ExpectScopeClosed(rig.runtime, 0);
        assert(rig.runtime.signals.simulatePhaseDone.load());
        assert(env.recorder.Equals({ "SimA" }));
    }

    static void Test_02_Dependency_A_Then_B()
    {
        LogTestBanner(__FUNCTION__);

        CallbackEnv env{};
        g_env = &env;

        TestRig rig{};
        rig.graph.scopeCount = 1;
        rig.graph.scopeToWorld = { 1002 };
        rig.graph.simulateNodeCount = 2;

        constexpr ExecToken kA = 1;
        constexpr ExecToken kB = 2;

        AddSource(rig.sources, kA, ExecPhase::Simulate, ExecLane::Parallel, ExecNodeKind::StaticSystem, &SimA, ExecNodeFlag_None, "SimA");
        AddSource(rig.sources, kB, ExecPhase::Simulate, ExecLane::Parallel, ExecNodeKind::StaticSystem, &SimB, ExecNodeFlag_None, "SimB");

        // node0 succ -> node1
        // node1 pred <- node0
        rig.graph.edges = { 1, 0 };

        rig.graph.nodes.push_back(
            MakeNode(
                0, 0,
                ExecPhase::Simulate, ExecLane::Parallel, ExecNodeKind::StaticSystem,
                kA,
                0, 0,
                0, 1));

        rig.graph.nodes.push_back(
            MakeNode(
                1, 0,
                ExecPhase::Simulate, ExecLane::Parallel, ExecNodeKind::StaticSystem,
                kB,
                1, 1,
                0, 0));

        rig.Finalize();

        TaskExecutor executor(2);
        const bool ok = executor.ExecuteFrame(rig.frame, rig.runtime, rig.sources);
        assert(ok);

        LogNodeState(rig.runtime, 0, "dependency-a");
        LogNodeState(rig.runtime, 1, "dependency-b");
        LogScopeState(rig.runtime, 0, "dependency-scope");
        LogRecorderState(env.recorder, "dependency");

        ExpectNodeState(rig.runtime, 0, ExecNodeState::Succeeded);
        ExpectNodeState(rig.runtime, 1, ExecNodeState::Succeeded);
        ExpectScopeClosed(rig.runtime, 0);

        assert(env.recorder.events.size() == 2);
        assert(env.recorder.events[0] == "SimA");
        assert(env.recorder.events[1] == "SimB");
    }

    static void Test_03_FanIn_A_B_To_C()
    {
        LogTestBanner(__FUNCTION__);

        CallbackEnv env{};
        g_env = &env;

        TestRig rig{};
        rig.graph.scopeCount = 1;
        rig.graph.scopeToWorld = { 1003 };
        rig.graph.simulateNodeCount = 3;

        constexpr ExecToken kA = 1;
        constexpr ExecToken kB = 2;
        constexpr ExecToken kC = 3;

        AddSource(rig.sources, kA, ExecPhase::Simulate, ExecLane::Parallel, ExecNodeKind::StaticSystem, &SimA, ExecNodeFlag_None, "SimA");
        AddSource(rig.sources, kB, ExecPhase::Simulate, ExecLane::Parallel, ExecNodeKind::StaticSystem, &SimB, ExecNodeFlag_None, "SimB");
        AddSource(rig.sources, kC, ExecPhase::Simulate, ExecLane::Parallel, ExecNodeKind::StaticSystem, &SimC, ExecNodeFlag_None, "SimC");

        // node0 succ = [2]
        // node1 succ = [2]
        // node2 pred = [0,1]
        rig.graph.edges = {
            2,
            2,
            0, 1
        };

        rig.graph.nodes.push_back(
            MakeNode(
                0, 0,
                ExecPhase::Simulate, ExecLane::Parallel, ExecNodeKind::StaticSystem,
                kA,
                0, 0,
                0, 1));

        rig.graph.nodes.push_back(
            MakeNode(
                1, 0,
                ExecPhase::Simulate, ExecLane::Parallel, ExecNodeKind::StaticSystem,
                kB,
                0, 0,
                1, 1));

        rig.graph.nodes.push_back(
            MakeNode(
                2, 0,
                ExecPhase::Simulate, ExecLane::Parallel, ExecNodeKind::StaticSystem,
                kC,
                2, 2,
                0, 0));

        rig.Finalize();

        TaskExecutor executor(3);
        const bool ok = executor.ExecuteFrame(rig.frame, rig.runtime, rig.sources);
        assert(ok);

        LogNodeState(rig.runtime, 0, "fanin-a");
        LogNodeState(rig.runtime, 1, "fanin-b");
        LogNodeState(rig.runtime, 2, "fanin-c");
        LogScopeState(rig.runtime, 0, "fanin-scope");
        LogRecorderState(env.recorder, "fanin");

        ExpectNodeState(rig.runtime, 0, ExecNodeState::Succeeded);
        ExpectNodeState(rig.runtime, 1, ExecNodeState::Succeeded);
        ExpectNodeState(rig.runtime, 2, ExecNodeState::Succeeded);

        assert(env.recorder.events.size() == 3);
        assert(env.recorder.events.back() == "SimC");
        assert(env.recorder.Count("SimC") == 1);
    }

    static void Test_04_ScopeFailure_Cancels_Successor()
    {
        LogTestBanner(__FUNCTION__);

        CallbackEnv env{};
        env.failA = true;
        g_env = &env;

        TestRig rig{};
        rig.graph.scopeCount = 1;
        rig.graph.scopeToWorld = { 1004 };
        rig.graph.simulateNodeCount = 2;

        constexpr ExecToken kA = 1;
        constexpr ExecToken kB = 2;

        AddSource(rig.sources, kA, ExecPhase::Simulate, ExecLane::Parallel, ExecNodeKind::StaticSystem, &SimA, ExecNodeFlag_None, "SimA");
        AddSource(rig.sources, kB, ExecPhase::Simulate, ExecLane::Parallel, ExecNodeKind::StaticSystem, &SimB, ExecNodeFlag_None, "SimB");

        // A -> B
        rig.graph.edges = { 1, 0 };

        rig.graph.nodes.push_back(
            MakeNode(
                0, 0,
                ExecPhase::Simulate, ExecLane::Parallel, ExecNodeKind::StaticSystem,
                kA,
                0, 0,
                0, 1));

        rig.graph.nodes.push_back(
            MakeNode(
                1, 0,
                ExecPhase::Simulate, ExecLane::Parallel, ExecNodeKind::StaticSystem,
                kB,
                1, 1,
                0, 0));

        rig.Finalize();

        TaskExecutor executor(2);
        const bool ok = executor.ExecuteFrame(rig.frame, rig.runtime, rig.sources);
        assert(ok);

        LogNodeState(rig.runtime, 0, "scope-failure-root");
        LogNodeState(rig.runtime, 1, "scope-failure-successor");
        LogScopeState(rig.runtime, 0, "scope-failure");
        LogRecorderState(env.recorder, "scope-failure");

        ExpectNodeState(rig.runtime, 0, ExecNodeState::Failed);

        const ExecNodeState succState = rig.runtime.TryGetNode(1)->state.load();
        assert(succState == ExecNodeState::Canceled ||
            succState == ExecNodeState::Skipped);

        ExpectScopeCanceled(rig.runtime, 0);
        assert(env.recorder.Count("SimA") == 1);
        assert(env.recorder.Count("SimB") == 0);
    }

    static void Test_05_SerialPhase_Order_Commit_Lifecycle_Reconcile()
    {
        LogTestBanner(__FUNCTION__);

        CallbackEnv env{};
        g_env = &env;

        TestRig rig{};
        rig.graph.scopeCount = 1;
        rig.graph.scopeToWorld = { 1005 };
        rig.graph.simulateNodeCount = 0;

        constexpr ExecToken kCommit = 10;
        constexpr ExecToken kLifecycle = 11;
        constexpr ExecToken kReconcile = 12;

        AddSource(rig.sources, kCommit, ExecPhase::Commit, ExecLane::Serial, ExecNodeKind::StructuralApply, &CommitA, ExecNodeFlag_None, "CommitA");
        AddSource(rig.sources, kLifecycle, ExecPhase::LifecycleFlush, ExecLane::Serial, ExecNodeKind::LifecycleFlush, &LifecycleA, ExecNodeFlag_None, "LifecycleA");
        AddSource(rig.sources, kReconcile, ExecPhase::Reconcile, ExecLane::Serial, ExecNodeKind::Reconcile, &ReconcileA, ExecNodeFlag_None, "ReconcileA");

        rig.graph.nodes.push_back(
            MakeNode(
                0, 0,
                ExecPhase::Commit,
                ExecLane::Serial,
                ExecNodeKind::StructuralApply,
                kCommit,
                0, 0, 0, 0));

        rig.graph.nodes.push_back(
            MakeNode(
                1, 0,
                ExecPhase::LifecycleFlush,
                ExecLane::Serial,
                ExecNodeKind::LifecycleFlush,
                kLifecycle,
                0, 0, 0, 0));

        rig.graph.nodes.push_back(
            MakeNode(
                2, 0,
                ExecPhase::Reconcile,
                ExecLane::Serial,
                ExecNodeKind::Reconcile,
                kReconcile,
                0, 0, 0, 0));

        rig.graph.serialExecutionOrder = { 0, 1, 2 };
        rig.graph.commitPlan = { 0, 1 };
        rig.graph.lifecycleFlushPlan = { 1, 1 };
        rig.graph.reconcilePlan = { 2, 1 };

        rig.Finalize();

        TaskExecutor executor(2);
        const bool ok = executor.ExecuteFrame(rig.frame, rig.runtime, rig.sources);
        assert(ok);

        LogNodeState(rig.runtime, 0, "serial-commit");
        LogNodeState(rig.runtime, 1, "serial-lifecycle");
        LogNodeState(rig.runtime, 2, "serial-reconcile");
        LogScopeState(rig.runtime, 0, "serial-scope");
        LogRecorderState(env.recorder, "serial-phase-order");

        ExpectNodeState(rig.runtime, 0, ExecNodeState::Succeeded);
        ExpectNodeState(rig.runtime, 1, ExecNodeState::Succeeded);
        ExpectNodeState(rig.runtime, 2, ExecNodeState::Succeeded);
        ExpectScopeClosed(rig.runtime, 0);

        assert(env.recorder.Equals({
            "CommitA",
            "LifecycleA",
            "ReconcileA"
            }));
    }

    static WorldDef MakeMinimalWorldDef()
    {
        WorldDef def{};
        def.id = WorldDefId::Plaza;
        def.name = "ExecutionOpsSmoke";
        def.topology = { WorldKind::Hub, WorldInstanceType::Instanced };
        def.entryPolicy = {
            CreationPolicy::CreateOnDemand,
            JoinPolicy::FreeJoin,
            0,
            true,
            false,
            std::nullopt,
            std::nullopt
        };
        def.map.resourceId = 0;
        def.map.defaultPlayerSpawnPointId = SpawnPointIds::None;
        def.map.navMesh = std::nullopt;
        def.map.terrainHeight = std::nullopt;
        def.spawn = { SpawnSetId::None, std::nullopt };
        def.progressRule = {
            WorldClearConditionType::None,
            WorldFailConditionType::None,
            WorldCompletionActionType::None,
            std::nullopt,
            false
        };
        def.linkRules = {};
        return def;
    }

    static WorldExecutionModel MakeSchedulerSmokeModel(
        const WorldExecutionModelKey key,
        const ExecToken commitToken,
        const ExecToken lifecycleToken,
        const ExecToken reconcileToken)
    {
        WorldExecutionModel model{};
        model.key = key;
        model.commitSources = { commitToken };
        model.lifecycleFlushSources = { lifecycleToken };
        model.reconcileSources = { reconcileToken };
        return model;
    }

    static void Test_06_ExecutionOps_Drives_WorldRuntime_Frame()
    {
        LogTestBanner(__FUNCTION__);

        WorldDef def = MakeMinimalWorldDef();

        WorldExecutionModel model{};
        model.key = 1;

        WorldRuntime runtime(WorldRuntimeCreateParams{
            &def,
            &model,
            nullptr,
            nullptr
        });

        const bool initOk = runtime.Initialize();
        assert(initOk);

        const bool beginOk = runtime.BeginFrame(1, 10.0, 0.016);
        assert(beginOk);

        const Entity reserved = runtime.ReserveEntity();
        assert(!reserved.IsNull());

        TestRig rig{};
        rig.ops = ExecutionOps(
            reinterpret_cast<WorldManager*>(0x1),
            reinterpret_cast<WorldRegistry*>(0x1),
            reinterpret_cast<WorldTransferService*>(0x1),
            reinterpret_cast<WorldAdmissionService*>(0x1),
            reinterpret_cast<PresenceManager*>(0x1));
        rig.graph.scopeCount = 1;
        rig.graph.scopeToWorld = { 2001 };
        rig.graph.simulateNodeCount = 0;

        constexpr ExecToken kCommit = 20;
        constexpr ExecToken kLifecycle = 21;
        constexpr ExecToken kReconcile = 22;

        AddSource(rig.sources, kCommit, ExecPhase::Commit, ExecLane::Serial, ExecNodeKind::StructuralApply, &CommitViaOps, ExecNodeFlag_None, "CommitViaOps");
        AddSource(rig.sources, kLifecycle, ExecPhase::LifecycleFlush, ExecLane::Serial, ExecNodeKind::LifecycleFlush, &LifecycleViaOps, ExecNodeFlag_None, "LifecycleViaOps");
        AddSource(rig.sources, kReconcile, ExecPhase::Reconcile, ExecLane::Serial, ExecNodeKind::Reconcile, &ReconcileViaOps, ExecNodeFlag_None, "ReconcileViaOps");

        rig.graph.nodes.push_back(
            MakeNode(
                0, 0,
                ExecPhase::Commit,
                ExecLane::Serial,
                ExecNodeKind::StructuralApply,
                kCommit,
                0, 0, 0, 0));

        rig.graph.nodes.push_back(
            MakeNode(
                1, 0,
                ExecPhase::LifecycleFlush,
                ExecLane::Serial,
                ExecNodeKind::LifecycleFlush,
                kLifecycle,
                0, 0, 0, 0));

        rig.graph.nodes.push_back(
            MakeNode(
                2, 0,
                ExecPhase::Reconcile,
                ExecLane::Serial,
                ExecNodeKind::Reconcile,
                kReconcile,
                0, 0, 0, 0));

        rig.graph.serialExecutionOrder = { 0, 1, 2 };
        rig.graph.commitPlan = { 0, 1 };
        rig.graph.lifecycleFlushPlan = { 1, 1 };
        rig.graph.reconcilePlan = { 2, 1 };

        rig.Finalize();
        rig.runtimeByScopeBacking[0] = &runtime;
        rig.frame.runtimeByScope = std::span<WorldRuntime*>(
            rig.runtimeByScopeBacking.data(),
            rig.runtimeByScopeBacking.size());

        TaskExecutor executor(2);
        const bool ok = executor.ExecuteFrame(rig.frame, rig.runtime, rig.sources);
        assert(ok);

        LogNodeState(rig.runtime, 0, "runtime-commit");
        LogNodeState(rig.runtime, 1, "runtime-lifecycle");
        LogNodeState(rig.runtime, 2, "runtime-reconcile");
        LogScopeState(rig.runtime, 0, "runtime-scope");
        LogWorldRuntimeState(runtime, "runtime-after-frame");

        assert(runtime.GetCommitState() == WorldRuntimeCommitState::CommitSucceeded);
        assert(runtime.GetLifecycleFlushState() == WorldRuntimeLifecycleFlushState::Flushed);
        assert(runtime.LifecycleOutbox().empty());
        assert(runtime.MakeView().IsAlive(reserved));

        runtime.Shutdown();
    }

    static void Test_07_ExecutionOps_CommitFailure_CancelsRemainingSerialPhases()
    {
        LogTestBanner(__FUNCTION__);

        WorldDef def = MakeMinimalWorldDef();

        WorldExecutionModel model{};
        model.key = 2;

        WorldRuntime runtime(WorldRuntimeCreateParams{
            &def,
            &model,
            nullptr,
            nullptr
        });

        const bool initOk = runtime.Initialize();
        assert(initOk);

        const bool beginOk = runtime.BeginFrame(2, 20.0, 0.016);
        assert(beginOk);

        const Entity reserved = runtime.ReserveEntity();
        assert(!reserved.IsNull());

        FaultProbeComponent faultProbe{};
        faultProbe.value = 7;
        runtime.DeferredAddComponent<FaultProbeComponent>(
            reserved,
            std::move(faultProbe));

        TestRig rig{};
        rig.ops = MakeDummyValidOps();
        rig.graph.scopeCount = 1;
        rig.graph.scopeToWorld = { 3001 };
        rig.graph.simulateNodeCount = 0;

        constexpr ExecToken kCommit = 30;
        constexpr ExecToken kLifecycle = 31;
        constexpr ExecToken kReconcile = 32;

        AddSource(rig.sources, kCommit, ExecPhase::Commit, ExecLane::Serial, ExecNodeKind::StructuralApply, &CommitViaOps, ExecNodeFlag_None, "CommitViaOps");
        AddSource(rig.sources, kLifecycle, ExecPhase::LifecycleFlush, ExecLane::Serial, ExecNodeKind::LifecycleFlush, &LifecycleViaOps, ExecNodeFlag_None, "LifecycleViaOps");
        AddSource(rig.sources, kReconcile, ExecPhase::Reconcile, ExecLane::Serial, ExecNodeKind::Reconcile, &ReconcileViaOps, ExecNodeFlag_None, "ReconcileViaOps");

        rig.graph.nodes.push_back(MakeNode(0, 0, ExecPhase::Commit, ExecLane::Serial, ExecNodeKind::StructuralApply, kCommit, 0, 0, 0, 0));
        rig.graph.nodes.push_back(MakeNode(1, 0, ExecPhase::LifecycleFlush, ExecLane::Serial, ExecNodeKind::LifecycleFlush, kLifecycle, 0, 0, 0, 0));
        rig.graph.nodes.push_back(MakeNode(2, 0, ExecPhase::Reconcile, ExecLane::Serial, ExecNodeKind::Reconcile, kReconcile, 0, 0, 0, 0));

        rig.graph.serialExecutionOrder = { 0, 1, 2 };
        rig.graph.commitPlan = { 0, 1 };
        rig.graph.lifecycleFlushPlan = { 1, 1 };
        rig.graph.reconcilePlan = { 2, 1 };

        rig.Finalize();
        rig.runtimeByScopeBacking[0] = &runtime;
        rig.frame.runtimeByScope = std::span<WorldRuntime*>(
            rig.runtimeByScopeBacking.data(),
            rig.runtimeByScopeBacking.size());

        TaskExecutor executor(2);
        const bool ok = executor.ExecuteFrame(rig.frame, rig.runtime, rig.sources);
        assert(ok);

        LogNodeState(rig.runtime, 0, "commit-failure-commit");
        LogNodeState(rig.runtime, 1, "commit-failure-lifecycle");
        LogNodeState(rig.runtime, 2, "commit-failure-reconcile");
        LogScopeState(rig.runtime, 0, "commit-failure-scope");
        LogWorldRuntimeState(runtime, "commit-failure-runtime");
        std::cout << "[TRACE] commit-failure faultMessage=" << runtime.GetFault().message << "\n";

        ExpectNodeState(rig.runtime, 0, ExecNodeState::Failed);
        ExpectNodeState(rig.runtime, 1, ExecNodeState::Canceled);
        ExpectNodeState(rig.runtime, 2, ExecNodeState::Canceled);
        ExpectScopeCanceled(rig.runtime, 0);

        assert(runtime.GetCommitState() == WorldRuntimeCommitState::CommitFailed);
        assert(runtime.GetLifecycleFlushState() == WorldRuntimeLifecycleFlushState::NotFlushed);
        assert(runtime.IsFaulted());
        assert(runtime.LifecycleOutbox().empty());

        runtime.Shutdown();
    }

    static void Test_08_ExecutionOps_DoubleLifecycleFlushFailsSecondNode()
    {
        LogTestBanner(__FUNCTION__);

        WorldDef def = MakeMinimalWorldDef();

        WorldExecutionModel model{};
        model.key = 3;

        WorldRuntime runtime(WorldRuntimeCreateParams{
            &def,
            &model,
            nullptr,
            nullptr
        });

        const bool initOk = runtime.Initialize();
        assert(initOk);

        const bool beginOk = runtime.BeginFrame(3, 30.0, 0.016);
        assert(beginOk);

        const Entity reserved = runtime.ReserveEntity();
        assert(!reserved.IsNull());

        TestRig rig{};
        rig.ops = MakeDummyValidOps();
        rig.graph.scopeCount = 1;
        rig.graph.scopeToWorld = { 3002 };
        rig.graph.simulateNodeCount = 0;

        constexpr ExecToken kCommit = 40;
        constexpr ExecToken kLifecycleA = 41;
        constexpr ExecToken kLifecycleB = 42;
        constexpr ExecToken kReconcile = 43;

        AddSource(rig.sources, kCommit, ExecPhase::Commit, ExecLane::Serial, ExecNodeKind::StructuralApply, &CommitViaOps, ExecNodeFlag_None, "CommitViaOps");
        AddSource(rig.sources, kLifecycleA, ExecPhase::LifecycleFlush, ExecLane::Serial, ExecNodeKind::LifecycleFlush, &LifecycleViaOps, ExecNodeFlag_None, "LifecycleViaOpsA");
        AddSource(rig.sources, kLifecycleB, ExecPhase::LifecycleFlush, ExecLane::Serial, ExecNodeKind::LifecycleFlush, &LifecycleViaOps, ExecNodeFlag_None, "LifecycleViaOpsB");
        AddSource(rig.sources, kReconcile, ExecPhase::Reconcile, ExecLane::Serial, ExecNodeKind::Reconcile, &ReconcileViaOps, ExecNodeFlag_None, "ReconcileViaOps");

        rig.graph.nodes.push_back(MakeNode(0, 0, ExecPhase::Commit, ExecLane::Serial, ExecNodeKind::StructuralApply, kCommit, 0, 0, 0, 0));
        rig.graph.nodes.push_back(MakeNode(1, 0, ExecPhase::LifecycleFlush, ExecLane::Serial, ExecNodeKind::LifecycleFlush, kLifecycleA, 0, 0, 0, 0));
        rig.graph.nodes.push_back(MakeNode(2, 0, ExecPhase::LifecycleFlush, ExecLane::Serial, ExecNodeKind::LifecycleFlush, kLifecycleB, 0, 0, 0, 0));
        rig.graph.nodes.push_back(MakeNode(3, 0, ExecPhase::Reconcile, ExecLane::Serial, ExecNodeKind::Reconcile, kReconcile, 0, 0, 0, 0));

        rig.graph.serialExecutionOrder = { 0, 1, 2, 3 };
        rig.graph.commitPlan = { 0, 1 };
        rig.graph.lifecycleFlushPlan = { 1, 2 };
        rig.graph.reconcilePlan = { 3, 1 };

        rig.Finalize();
        rig.runtimeByScopeBacking[0] = &runtime;
        rig.frame.runtimeByScope = std::span<WorldRuntime*>(
            rig.runtimeByScopeBacking.data(),
            rig.runtimeByScopeBacking.size());

        TaskExecutor executor(2);
        const bool ok = executor.ExecuteFrame(rig.frame, rig.runtime, rig.sources);
        assert(ok);

        LogNodeState(rig.runtime, 0, "double-flush-commit");
        LogNodeState(rig.runtime, 1, "double-flush-lifecycle-first");
        LogNodeState(rig.runtime, 2, "double-flush-lifecycle-second");
        LogNodeState(rig.runtime, 3, "double-flush-reconcile");
        LogScopeState(rig.runtime, 0, "double-flush-scope");
        LogWorldRuntimeState(runtime, "double-flush-runtime");
        std::cout << "[TRACE] double-flush faultMessage=" << runtime.GetFault().message << "\n";

        ExpectNodeState(rig.runtime, 0, ExecNodeState::Succeeded);
        ExpectNodeState(rig.runtime, 1, ExecNodeState::Succeeded);
        ExpectNodeState(rig.runtime, 2, ExecNodeState::Failed);
        ExpectNodeState(rig.runtime, 3, ExecNodeState::Canceled);
        ExpectScopeCanceled(rig.runtime, 0);

        assert(runtime.GetCommitState() == WorldRuntimeCommitState::CommitSucceeded);
        assert(runtime.GetLifecycleFlushState() == WorldRuntimeLifecycleFlushState::Flushed);
        assert(runtime.IsFaulted());
        assert(runtime.MakeView().IsAlive(reserved));

        runtime.Shutdown();
    }

    static void Test_09_MultiScope_IsolatedSerialPipeline()
    {
        LogTestBanner(__FUNCTION__);

        WorldDef defA = MakeMinimalWorldDef();
        defA.name = "ExecutionOpsMultiScopeA";

        WorldDef defB = MakeMinimalWorldDef();
        defB.name = "ExecutionOpsMultiScopeB";

        WorldExecutionModel modelA{};
        modelA.key = 4;

        WorldExecutionModel modelB{};
        modelB.key = 5;

        WorldRuntime runtimeA(WorldRuntimeCreateParams{
            &defA,
            &modelA,
            nullptr,
            nullptr
        });

        WorldRuntime runtimeB(WorldRuntimeCreateParams{
            &defB,
            &modelB,
            nullptr,
            nullptr
        });

        assert(runtimeA.Initialize());
        assert(runtimeB.Initialize());
        assert(runtimeA.BeginFrame(4, 40.0, 0.016));
        assert(runtimeB.BeginFrame(4, 40.0, 0.016));

        const Entity reservedA = runtimeA.ReserveEntity();
        const Entity reservedB0 = runtimeB.ReserveEntity();
        const Entity reservedB1 = runtimeB.ReserveEntity();
        assert(!reservedA.IsNull());
        assert(!reservedB0.IsNull());
        assert(!reservedB1.IsNull());

        TestRig rig{};
        rig.ops = MakeDummyValidOps();
        rig.graph.scopeCount = 2;
        rig.graph.scopeToWorld = { 4001, 4002 };
        rig.graph.simulateNodeCount = 0;

        constexpr ExecToken kCommit = 50;
        constexpr ExecToken kLifecycle = 51;
        constexpr ExecToken kReconcile = 52;

        AddSource(rig.sources, kCommit, ExecPhase::Commit, ExecLane::Serial, ExecNodeKind::StructuralApply, &CommitViaOps, ExecNodeFlag_None, "CommitViaOps");
        AddSource(rig.sources, kLifecycle, ExecPhase::LifecycleFlush, ExecLane::Serial, ExecNodeKind::LifecycleFlush, &LifecycleViaOps, ExecNodeFlag_None, "LifecycleViaOps");
        AddSource(rig.sources, kReconcile, ExecPhase::Reconcile, ExecLane::Serial, ExecNodeKind::Reconcile, &ReconcileViaOps, ExecNodeFlag_None, "ReconcileViaOps");

        rig.graph.nodes.push_back(MakeNode(0, 0, ExecPhase::Commit, ExecLane::Serial, ExecNodeKind::StructuralApply, kCommit, 0, 0, 0, 0));
        rig.graph.nodes.push_back(MakeNode(1, 1, ExecPhase::Commit, ExecLane::Serial, ExecNodeKind::StructuralApply, kCommit, 0, 0, 0, 0));
        rig.graph.nodes.push_back(MakeNode(2, 0, ExecPhase::LifecycleFlush, ExecLane::Serial, ExecNodeKind::LifecycleFlush, kLifecycle, 0, 0, 0, 0));
        rig.graph.nodes.push_back(MakeNode(3, 1, ExecPhase::LifecycleFlush, ExecLane::Serial, ExecNodeKind::LifecycleFlush, kLifecycle, 0, 0, 0, 0));
        rig.graph.nodes.push_back(MakeNode(4, 0, ExecPhase::Reconcile, ExecLane::Serial, ExecNodeKind::Reconcile, kReconcile, 0, 0, 0, 0));
        rig.graph.nodes.push_back(MakeNode(5, 1, ExecPhase::Reconcile, ExecLane::Serial, ExecNodeKind::Reconcile, kReconcile, 0, 0, 0, 0));

        rig.graph.serialExecutionOrder = { 0, 1, 2, 3, 4, 5 };
        rig.graph.commitPlan = { 0, 2 };
        rig.graph.lifecycleFlushPlan = { 2, 2 };
        rig.graph.reconcilePlan = { 4, 2 };

        rig.Finalize();
        rig.runtimeByScopeBacking[0] = &runtimeA;
        rig.runtimeByScopeBacking[1] = &runtimeB;
        rig.frame.runtimeByScope = std::span<WorldRuntime*>(
            rig.runtimeByScopeBacking.data(),
            rig.runtimeByScopeBacking.size());

        TaskExecutor executor(2);
        const bool ok = executor.ExecuteFrame(rig.frame, rig.runtime, rig.sources);
        assert(ok);

        LogNodeState(rig.runtime, 0, "multi-scope-commit-a");
        LogNodeState(rig.runtime, 1, "multi-scope-commit-b");
        LogNodeState(rig.runtime, 2, "multi-scope-lifecycle-a");
        LogNodeState(rig.runtime, 3, "multi-scope-lifecycle-b");
        LogNodeState(rig.runtime, 4, "multi-scope-reconcile-a");
        LogNodeState(rig.runtime, 5, "multi-scope-reconcile-b");
        LogScopeState(rig.runtime, 0, "multi-scope-a");
        LogScopeState(rig.runtime, 1, "multi-scope-b");
        LogWorldRuntimeState(runtimeA, "multi-scope-runtime-a");
        LogWorldRuntimeState(runtimeB, "multi-scope-runtime-b");

        for (ExecNodeId nodeId = 0; nodeId < 6; ++nodeId)
            ExpectNodeState(rig.runtime, nodeId, ExecNodeState::Succeeded);

        ExpectScopeClosed(rig.runtime, 0);
        ExpectScopeClosed(rig.runtime, 1);

        assert(runtimeA.MakeView().IsAlive(reservedA));
        assert(runtimeB.MakeView().IsAlive(reservedB0));
        assert(runtimeB.MakeView().IsAlive(reservedB1));
        assert(runtimeA.MakeView().AliveEntities().size() == 1);
        assert(runtimeB.MakeView().AliveEntities().size() == 2);
        assert(runtimeA.LifecycleOutbox().empty());
        assert(runtimeB.LifecycleOutbox().empty());

        runtimeA.Shutdown();
        runtimeB.Shutdown();
    }

    static void Test_10_WorldScheduler_Runs_Frame_FromBootstrapPipeline()
    {
        LogTestBanner(__FUNCTION__);

        constexpr ExecToken kCommit = 60;
        constexpr ExecToken kLifecycle = 61;
        constexpr ExecToken kReconcile = 62;
        constexpr WorldExecutionModelKey kModelKey = 100;

        TestWorldInstanceFactory factory{};
        WorldExecutionModelRegistry executionModelRegistry{};
        WorldRegistry worldRegistry(factory, executionModelRegistry);

        StaticDefinitionProvider provider{};
        provider.sources.push_back(MakeSourceDesc(
            kCommit,
            ExecPhase::Commit,
            ExecLane::Serial,
            ExecNodeKind::StructuralApply,
            &CommitViaOps,
            ExecNodeFlag_None,
            "CommitViaOps"));
        provider.sources.push_back(MakeSourceDesc(
            kLifecycle,
            ExecPhase::LifecycleFlush,
            ExecLane::Serial,
            ExecNodeKind::LifecycleFlush,
            &LifecycleViaOps,
            ExecNodeFlag_None,
            "LifecycleViaOps"));
        provider.sources.push_back(MakeSourceDesc(
            kReconcile,
            ExecPhase::Reconcile,
            ExecLane::Serial,
            ExecNodeKind::Reconcile,
            &ReconcileViaOps,
            ExecNodeFlag_None,
            "ReconcileViaOps"));

        provider.models.push_back(MakeSchedulerSmokeModel(
            kModelKey,
            kCommit,
            kLifecycle,
            kReconcile));

        WorldDef def = MakeMinimalWorldDef();
        def.name = "SchedulerSmoke";
        def.executionModelKey = kModelKey;
        provider.defs.push_back(def);

        ExecutionSourceRegistry sourceRegistry{};
        WorldTransferProfileRegistry transferProfileRegistry{};
        const bool bootstrapOk = BootstrapWorldDefinitions(
            provider,
            sourceRegistry,
            executionModelRegistry,
            transferProfileRegistry,
            worldRegistry);
        assert(bootstrapOk);

        WorldManager worldManager(worldRegistry);
        const WorldId worldId = worldManager.RegisterPreCreatedWorld(def.id, 1);
        assert(worldId.IsValid());

        WorldInstance* world = worldRegistry.FindWorld(worldId);
        assert(world != nullptr);
        assert(world->Initialize());

        WorldRuntime& runtime = world->GetRuntime();
        runtime.ClearLifecycleOutbox();

        const Entity reserved = runtime.ReserveEntity();
        assert(!reserved.IsNull());

        worldManager.FlushLifecycle(0.0);
        worldManager.FlushLifecycle(0.0);

        const WorldInstanceRecord* record = worldManager.FindRecord(worldId);
        assert(record != nullptr);
        assert(record->IsRunnable());

        ExecutionGraphBuilder graphBuilder{};
        ExecutionGraphBuildPolicy buildPolicy{};
        TaskExecutor executor(2);
        ExecutionOps ops(
            &worldManager,
            &worldRegistry,
            reinterpret_cast<WorldTransferService*>(0x1),
            reinterpret_cast<WorldAdmissionService*>(0x1),
            reinterpret_cast<PresenceManager*>(0x1));

        WorldScheduler scheduler(
            worldManager,
            worldRegistry,
            executionModelRegistry,
            sourceRegistry,
            graphBuilder,
            buildPolicy,
            executor,
            ops);

        WorldSchedulerFrameResult result{};
        const bool runOk = scheduler.RunFrame(
            WorldSchedulerFrameParams{
                1,
                100.0,
                0.016
            },
            result);

        assert(runOk);
        LogSchedulerResult(result, scheduler.GetLastBuildResult(), "scheduler-smoke");
        LogWorldRuntimeState(runtime, "scheduler-smoke-runtime");

        assert(result.success);
        assert(result.graphBuilt);
        assert(result.executed);
        assert(result.selectedWorldCount == 1);
        assert(scheduler.GetLastBuildResult().success);
        assert(!runtime.IsFaulted());
        assert(runtime.GetCommitState() == WorldRuntimeCommitState::CommitSucceeded);
        assert(runtime.GetLifecycleFlushState() == WorldRuntimeLifecycleFlushState::Flushed);
        assert(runtime.LifecycleOutbox().empty());
        assert(runtime.MakeView().IsAlive(reserved));
        assert(runtime.MakeView().AliveEntities().size() == 1);
    }

    static void Test_11_ExecutionModelRegistration_Fails_On_SourcePhaseMismatch()
    {
        LogTestBanner(__FUNCTION__);

        constexpr ExecToken kToken = 70;

        ExecutionSourceRegistry sourceRegistry{};
        const bool sourceOk = sourceRegistry.Register(MakeSourceDesc(
            kToken,
            ExecPhase::Simulate,
            ExecLane::Parallel,
            ExecNodeKind::StaticSystem,
            &SimA,
            ExecNodeFlag_None,
            "PhaseMismatchSource"));
        assert(sourceOk);

        WorldExecutionModel model{};
        model.key = 200;
        model.commitSources = { kToken };

        WorldExecutionModelRegistry executionModelRegistry{};
        const bool registerOk =
            executionModelRegistry.Register(model, sourceRegistry);

        std::cout
            << "[TRACE] phase-mismatch registerOk="
            << (registerOk ? "true" : "false")
            << "\n";

        assert(!registerOk);
        assert(!executionModelRegistry.Has(model.key));
    }

    static void Test_12_WorldRegistry_CreateWorld_Fails_When_ExecutionModelResolveFails()
    {
        LogTestBanner(__FUNCTION__);

        TestWorldInstanceFactory factory{};
        WorldExecutionModelRegistry executionModelRegistry{};
        WorldRegistry worldRegistry(factory, executionModelRegistry);

        WorldDef def = MakeMinimalWorldDef();
        def.name = "ResolveFailSmoke";
        def.executionModelKey = 9999;

        const bool registerOk = worldRegistry.RegisterWorldDef(def);
        std::cout
            << "[TRACE] missing-model registerWorldDef="
            << (registerOk ? "true" : "false")
            << "\n";
        assert(!registerOk);

        WorldInstance* instance = worldRegistry.CreateWorld(def, 1);
        std::cout
            << "[TRACE] missing-model createWorld="
            << (instance != nullptr ? "non-null" : "null")
            << "\n";
        assert(instance == nullptr);
    }
}

int main()
{
    Test_01_SingleSimulateSuccess();
    std::cout << "[PASS] Test_01_SingleSimulateSuccess\n";

    Test_02_Dependency_A_Then_B();
    std::cout << "[PASS] Test_02_Dependency_A_Then_B\n";

    Test_03_FanIn_A_B_To_C();
    std::cout << "[PASS] Test_03_FanIn_A_B_To_C\n";

    Test_04_ScopeFailure_Cancels_Successor();
    std::cout << "[PASS] Test_04_ScopeFailure_Cancels_Successor\n";

    Test_05_SerialPhase_Order_Commit_Lifecycle_Reconcile();
    std::cout << "[PASS] Test_05_SerialPhase_Order_Commit_Lifecycle_Reconcile\n";

    Test_06_ExecutionOps_Drives_WorldRuntime_Frame();
    std::cout << "[PASS] Test_06_ExecutionOps_Drives_WorldRuntime_Frame\n";

    Test_07_ExecutionOps_CommitFailure_CancelsRemainingSerialPhases();
    std::cout << "[PASS] Test_07_ExecutionOps_CommitFailure_CancelsRemainingSerialPhases\n";

    Test_08_ExecutionOps_DoubleLifecycleFlushFailsSecondNode();
    std::cout << "[PASS] Test_08_ExecutionOps_DoubleLifecycleFlushFailsSecondNode\n";

    Test_09_MultiScope_IsolatedSerialPipeline();
    std::cout << "[PASS] Test_09_MultiScope_IsolatedSerialPipeline\n";

    Test_10_WorldScheduler_Runs_Frame_FromBootstrapPipeline();
    std::cout << "[PASS] Test_10_WorldScheduler_Runs_Frame_FromBootstrapPipeline\n";

    Test_11_ExecutionModelRegistration_Fails_On_SourcePhaseMismatch();
    std::cout << "[PASS] Test_11_ExecutionModelRegistration_Fails_On_SourcePhaseMismatch\n";

    Test_12_WorldRegistry_CreateWorld_Fails_When_ExecutionModelResolveFails();
    std::cout << "[PASS] Test_12_WorldRegistry_CreateWorld_Fails_When_ExecutionModelResolveFails\n";

    std::cout << "All TaskExecutor tests passed.\n";
    return 0;
}
