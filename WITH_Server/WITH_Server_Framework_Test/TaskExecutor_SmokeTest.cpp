#include "pch.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
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
#include "ExecutionOps.h"

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

    static CallbackEnv* g_env = nullptr;

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

    static ExecutionOps MakeDummyValidOps()
    {
        // 테스트 callback에서는 ops를 실제로 사용하지 않는다는 전제의 더미다.
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

        ExpectNodeState(rig.runtime, 0, ExecNodeState::Succeeded);
        ExpectScopeClosed(rig.runtime, 0);
        assert(rig.runtime.signals.simulatePhaseDone.load());
        assert(env.recorder.Equals({ "SimA" }));
    }

    static void Test_02_Dependency_A_Then_B()
    {
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

        ExpectNodeState(rig.runtime, 0, ExecNodeState::Succeeded);
        ExpectNodeState(rig.runtime, 1, ExecNodeState::Succeeded);
        ExpectScopeClosed(rig.runtime, 0);

        assert(env.recorder.events.size() == 2);
        assert(env.recorder.events[0] == "SimA");
        assert(env.recorder.events[1] == "SimB");
    }

    static void Test_03_FanIn_A_B_To_C()
    {
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

        ExpectNodeState(rig.runtime, 0, ExecNodeState::Succeeded);
        ExpectNodeState(rig.runtime, 1, ExecNodeState::Succeeded);
        ExpectNodeState(rig.runtime, 2, ExecNodeState::Succeeded);

        assert(env.recorder.events.size() == 3);
        assert(env.recorder.events.back() == "SimC");
        assert(env.recorder.Count("SimC") == 1);
    }

    static void Test_04_ScopeFailure_Cancels_Successor()
    {
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

    std::cout << "All TaskExecutor tests passed.\n";
    return 0;
}