#include "pch.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "AutoSystemBridge.h"
#include "ExecutionContextTypes.h"
#include "ExecutionGraphBuildPolicy.h"
#include "ExecutionGraphBuilder.h"
#include "ExecutionOps.h"
#include "ExecutionRuntimeTypes.h"
#include "SystemMetaStorage.h"
#include "TaskExecutor.h"
#include "WorldDef.h"
#include "WorldExecutionModelTypes.h"
#include "WorldFrameSelectionTypes.h"
#include "WorldRuntime.h"
#include "WorldRuntimeTypes.h"

namespace
{
    struct TransformComponent {};
    struct AICommandComponent {};
    struct PlayerCommandComponent {};
    struct ActionStateComponent {};
    struct LocomotionStateComponent {};
    struct AnimationStateComponent {};
    struct BossPhaseComponent {};
    struct OrderingHintResource {};

    struct Phase1SmokeState
    {
        std::atomic<int> activeSnapshotReaders{ 0 };
        std::atomic<int> maxSnapshotReaders{ 0 };

        std::atomic<bool> perceptionA{ false };
        std::atomic<bool> perceptionB{ false };
        std::atomic<bool> aiCommand{ false };
        std::atomic<bool> playerCommand{ false };
        std::atomic<bool> actionResolved{ false };
        std::atomic<bool> locomotionResolved{ false };
        std::atomic<bool> animationResolved{ false };
        std::atomic<bool> bossPhaseResolved{ false };
        std::atomic<bool> spawnRequested{ false };
        std::atomic<bool> orderingBeforeRan{ false };
        std::atomic<bool> orderingAfterRan{ false };
        std::atomic<bool> presentationRan{ false };
        std::atomic<bool> presentationOnMainThread{ false };
        std::atomic<bool> commitProbeRan{ false };
        std::atomic<bool> lifecycleProbeRan{ false };
        std::atomic<bool> reconcileProbeRan{ false };

        std::thread::id mainThreadId{};

        std::mutex mutex;
        std::vector<std::string> trace;
        std::vector<std::string> failures;

        void PushTrace(std::string event)
        {
            std::lock_guard lock{ mutex };
            trace.push_back(std::move(event));
        }

        void Fail(std::string message)
        {
            std::lock_guard lock{ mutex };
            failures.push_back(std::move(message));
        }

        void UpdateMaxReaders(int value)
        {
            int previous = maxSnapshotReaders.load(std::memory_order_relaxed);
            while (previous < value &&
                !maxSnapshotReaders.compare_exchange_weak(
                    previous,
                    value,
                    std::memory_order_relaxed))
            {
            }
        }
    };

    Phase1SmokeState* g_state{ nullptr };

    void Check(bool condition, const char* message)
    {
        if (!condition)
            throw std::runtime_error(message);
    }

    void CheckNoRecordedFailures(Phase1SmokeState& state)
    {
        std::lock_guard lock{ state.mutex };
        if (state.failures.empty())
            return;

        std::ostringstream oss;
        oss << "Phase1 smoke recorded " << state.failures.size() << " failure(s):";
        for (const std::string& failure : state.failures)
            oss << "\n  - " << failure;

        throw std::runtime_error(oss.str());
    }

    WorldDef MakeSmokeWorldDef(WorldExecutionModelKey modelKey)
    {
        WorldDef def{};
        def.id = WorldDefId::Plaza;
        def.name = "Phase1AutoSystemBridgeSmoke";
        def.topology = { WorldKind::Dungeon, WorldInstanceType::Instanced };
        def.entryPolicy = {
            CreationPolicy::CreateOnDemand,
            JoinPolicy::FreeJoin,
            16,
            true,
            false,
            std::nullopt,
            std::nullopt
        };
        def.map.resourceId = 0;
        def.map.defaultPlayerSpawnPointId = SpawnPointIds::None;
        def.spawn = { SpawnSetId::None, std::nullopt };
        def.progressRule = {
            WorldClearConditionType::None,
            WorldFailConditionType::None,
            WorldCompletionActionType::None,
            std::nullopt,
            false
        };
        def.executionModelKey = modelKey;
        return def;
    }

    ExecutionOps MakeDummyValidOps()
    {
        return ExecutionOps(
            reinterpret_cast<WorldManager*>(0x1),
            reinterpret_cast<WorldRegistry*>(0x1),
            reinterpret_cast<WorldTransferService*>(0x1),
            reinterpret_cast<WorldAdmissionService*>(0x1),
            reinterpret_cast<NetIdRegistry*>(0x1),
            reinterpret_cast<PresenceManager*>(0x1));
    }

    void RunSnapshotReader(
        Phase1SmokeState& state,
        std::atomic<bool>& completionFlag,
        const char* name)
    {
        const int active =
            state.activeSnapshotReaders.fetch_add(1, std::memory_order_acq_rel) + 1;
        state.UpdateMaxReaders(active);
        state.PushTrace(std::string(name) + ":begin");

        std::this_thread::sleep_for(std::chrono::milliseconds(40));

        completionFlag.store(true, std::memory_order_release);
        state.PushTrace(std::string(name) + ":end");
        state.activeSnapshotReaders.fetch_sub(1, std::memory_order_acq_rel);
    }

    class PerceptionReadASystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<PerceptionReadASystem>(),
            "PerceptionReadASystem",
            std::array{ ReadSnapshot(ComponentRes<TransformComponent>()) });

        explicit PerceptionReadASystem(Phase1SmokeState* state) : _state(state) {}

        const ExecMeta& Meta() const override { return kMeta.meta; }

        void Execute(SystemContext& ctx) override
        {
            (void)ctx;
            RunSnapshotReader(*_state, _state->perceptionA, "perception-a");
        }

    private:
        Phase1SmokeState* _state;
    };

    class PerceptionReadBSystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<PerceptionReadBSystem>(),
            "PerceptionReadBSystem",
            std::array{ ReadSnapshot(ComponentRes<TransformComponent>()) });

        explicit PerceptionReadBSystem(Phase1SmokeState* state) : _state(state) {}

        const ExecMeta& Meta() const override { return kMeta.meta; }

        void Execute(SystemContext& ctx) override
        {
            (void)ctx;
            RunSnapshotReader(*_state, _state->perceptionB, "perception-b");
        }

    private:
        Phase1SmokeState* _state;
    };

    class AICommandSystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<AICommandSystem>(),
            "AICommandSystem",
            std::array{
                ReadSnapshot(ComponentRes<TransformComponent>()),
                WriteImmediate(ComponentRes<AICommandComponent>())
            });

        explicit AICommandSystem(Phase1SmokeState* state) : _state(state) {}

        const ExecMeta& Meta() const override { return kMeta.meta; }

        void Execute(SystemContext& ctx) override
        {
            (void)ctx;
            _state->PushTrace("ai-command");
            _state->aiCommand.store(true, std::memory_order_release);
        }

    private:
        Phase1SmokeState* _state;
    };

    class PlayerCommandSystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<PlayerCommandSystem>(),
            "PlayerCommandSystem",
            std::array{ WriteImmediate(ComponentRes<PlayerCommandComponent>()) });

        explicit PlayerCommandSystem(Phase1SmokeState* state) : _state(state) {}

        const ExecMeta& Meta() const override { return kMeta.meta; }

        void Execute(SystemContext& ctx) override
        {
            (void)ctx;
            _state->PushTrace("player-command");
            _state->playerCommand.store(true, std::memory_order_release);
        }

    private:
        Phase1SmokeState* _state;
    };

    class ResolveActionStateSystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<ResolveActionStateSystem>(),
            "ResolveActionStateSystem",
            std::array{
                ReadImmediate(ComponentRes<AICommandComponent>()),
                ReadImmediate(ComponentRes<PlayerCommandComponent>()),
                WriteImmediate(ComponentRes<ActionStateComponent>())
            });

        explicit ResolveActionStateSystem(Phase1SmokeState* state) : _state(state) {}

        const ExecMeta& Meta() const override { return kMeta.meta; }

        void Execute(SystemContext& ctx) override
        {
            (void)ctx;
            if (!_state->aiCommand.load(std::memory_order_acquire))
                _state->Fail("ResolveActionStateSystem ran before AICommandSystem.");
            if (!_state->playerCommand.load(std::memory_order_acquire))
                _state->Fail("ResolveActionStateSystem ran before PlayerCommandSystem.");

            _state->PushTrace("resolve-action");
            _state->actionResolved.store(true, std::memory_order_release);
        }

    private:
        Phase1SmokeState* _state;
    };

    class ResolveLocomotionSystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<ResolveLocomotionSystem>(),
            "ResolveLocomotionSystem",
            std::array{
                ReadImmediate(ComponentRes<ActionStateComponent>()),
                WriteImmediate(ComponentRes<LocomotionStateComponent>()),
                WriteImmediate(ComponentRes<TransformComponent>())
            });

        explicit ResolveLocomotionSystem(Phase1SmokeState* state) : _state(state) {}

        const ExecMeta& Meta() const override { return kMeta.meta; }

        void Execute(SystemContext& ctx) override
        {
            (void)ctx;
            if (!_state->actionResolved.load(std::memory_order_acquire))
                _state->Fail("ResolveLocomotionSystem ran before ResolveActionStateSystem.");

            _state->PushTrace("resolve-locomotion");
            _state->locomotionResolved.store(true, std::memory_order_release);
        }

    private:
        Phase1SmokeState* _state;
    };

    class ResolveAnimationSystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<ResolveAnimationSystem>(),
            "ResolveAnimationSystem",
            std::array{
                ReadImmediate(ComponentRes<ActionStateComponent>()),
                WriteImmediate(ComponentRes<AnimationStateComponent>())
            });

        explicit ResolveAnimationSystem(Phase1SmokeState* state) : _state(state) {}

        const ExecMeta& Meta() const override { return kMeta.meta; }

        void Execute(SystemContext& ctx) override
        {
            (void)ctx;
            if (!_state->actionResolved.load(std::memory_order_acquire))
                _state->Fail("ResolveAnimationSystem ran before ResolveActionStateSystem.");

            _state->PushTrace("resolve-animation");
            _state->animationResolved.store(true, std::memory_order_release);
        }

    private:
        Phase1SmokeState* _state;
    };

    class BossPhaseSystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<BossPhaseSystem>(),
            "BossPhaseSystem",
            std::array{
                ReadImmediate(ComponentRes<ActionStateComponent>()),
                ReadImmediate(ComponentRes<AnimationStateComponent>()),
                WriteImmediate(ComponentRes<BossPhaseComponent>())
            });

        explicit BossPhaseSystem(Phase1SmokeState* state) : _state(state) {}

        const ExecMeta& Meta() const override { return kMeta.meta; }

        void Execute(SystemContext& ctx) override
        {
            (void)ctx;
            if (!_state->animationResolved.load(std::memory_order_acquire))
                _state->Fail("BossPhaseSystem ran before ResolveAnimationSystem runsAfter hint.");

            _state->PushTrace("boss-phase");
            _state->bossPhaseResolved.store(true, std::memory_order_release);
        }

    private:
        Phase1SmokeState* _state;
    };

    class SpawnRequestSystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<SpawnRequestSystem>(),
            "SpawnRequestSystem",
            std::array{
                ReadImmediate(ComponentRes<BossPhaseComponent>()),
                WriteDeferred(CommandBufferRes())
            });

        explicit SpawnRequestSystem(Phase1SmokeState* state) : _state(state) {}

        const ExecMeta& Meta() const override { return kMeta.meta; }

        void Execute(SystemContext& ctx) override
        {
            if (!_state->bossPhaseResolved.load(std::memory_order_acquire))
                _state->Fail("SpawnRequestSystem ran before BossPhaseSystem runsAfter hint.");

            const Entity entity = ctx.runtime.ReserveEntity();
            if (entity.IsNull())
                _state->Fail("SpawnRequestSystem failed to reserve an entity.");

            _state->PushTrace("spawn-request");
            _state->spawnRequested.store(true, std::memory_order_release);
        }

    private:
        Phase1SmokeState* _state;
    };

    class OrderingHintTargetSystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<OrderingHintTargetSystem>(),
            "OrderingHintTargetSystem",
            std::array{ ReadSnapshot(ExternalRes<OrderingHintResource>()) });

        explicit OrderingHintTargetSystem(Phase1SmokeState* state) : _state(state) {}

        const ExecMeta& Meta() const override { return kMeta.meta; }

        void Execute(SystemContext& ctx) override
        {
            (void)ctx;
            if (!_state->orderingBeforeRan.load(std::memory_order_acquire))
                _state->Fail("OrderingHintTargetSystem ran before its runsBefore predecessor.");

            _state->PushTrace("ordering-hint-target");
            _state->orderingAfterRan.store(true, std::memory_order_release);
        }

    private:
        Phase1SmokeState* _state;
    };

    class OrderingHintSourceSystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<OrderingHintSourceSystem>(),
            "OrderingHintSourceSystem",
            std::array{ ReadSnapshot(ExternalRes<OrderingHintResource>()) },
            std::array{ SysTag<OrderingHintTargetSystem>() });

        explicit OrderingHintSourceSystem(Phase1SmokeState* state) : _state(state) {}

        const ExecMeta& Meta() const override { return kMeta.meta; }

        void Execute(SystemContext& ctx) override
        {
            (void)ctx;
            _state->PushTrace("ordering-hint-source");
            _state->orderingBeforeRan.store(true, std::memory_order_release);
        }

    private:
        Phase1SmokeState* _state;
    };

    ExecCallResult PresentationMainThread(NodeExecContext& ctx)
    {
        (void)ctx;
        if (g_state == nullptr)
            return ExecCallResult::Failed;

        if (!g_state->animationResolved.load(std::memory_order_acquire))
            g_state->Fail("PresentationMainThread ran before ResolveAnimationSystem.");
        if (!g_state->bossPhaseResolved.load(std::memory_order_acquire))
            g_state->Fail("PresentationMainThread ran before BossPhaseSystem.");

        const bool onMainThread = std::this_thread::get_id() == g_state->mainThreadId;
        g_state->presentationOnMainThread.store(onMainThread, std::memory_order_release);
        if (!onMainThread)
            g_state->Fail("PresentationMainThread did not run on the caller/main thread.");

        g_state->PushTrace("presentation-main-thread");
        g_state->presentationRan.store(true, std::memory_order_release);
        return ExecCallResult::Success;
    }

    ExecCallResult CommitBoundaryProbe(NodeExecContext& ctx)
    {
        WorldRuntime* runtime = ctx.TryGetRuntime();
        if (g_state == nullptr || runtime == nullptr)
            return ExecCallResult::Failed;

        if (!g_state->presentationRan.load(std::memory_order_acquire))
            g_state->Fail("CommitBoundaryProbe ran before simulate presentation completed.");
        if (!g_state->spawnRequested.load(std::memory_order_acquire))
            g_state->Fail("CommitBoundaryProbe ran before SpawnRequestSystem.");
        if (runtime->GetCommitState() != WorldRuntimeCommitState::NotCommitted)
            g_state->Fail("CommitBoundaryProbe observed an unexpected commit state.");

        g_state->PushTrace("commit-probe");
        g_state->commitProbeRan.store(true, std::memory_order_release);
        return ExecCallResult::Success;
    }

    ExecCallResult LifecycleBoundaryProbe(NodeExecContext& ctx)
    {
        WorldRuntime* runtime = ctx.TryGetRuntime();
        if (g_state == nullptr || runtime == nullptr)
            return ExecCallResult::Failed;

        if (!g_state->commitProbeRan.load(std::memory_order_acquire))
            g_state->Fail("LifecycleBoundaryProbe ran before CommitBoundaryProbe.");
        if (runtime->GetCommitState() != WorldRuntimeCommitState::CommitSucceeded)
            g_state->Fail("LifecycleBoundaryProbe ran before framework commit scope flush.");

        g_state->PushTrace("lifecycle-probe");
        g_state->lifecycleProbeRan.store(true, std::memory_order_release);
        return ExecCallResult::Success;
    }

    ExecCallResult ReconcileBoundaryProbe(NodeExecContext& ctx)
    {
        WorldRuntime* runtime = ctx.TryGetRuntime();
        if (g_state == nullptr || runtime == nullptr)
            return ExecCallResult::Failed;

        if (!g_state->lifecycleProbeRan.load(std::memory_order_acquire))
            g_state->Fail("ReconcileBoundaryProbe ran before LifecycleBoundaryProbe.");
        if (runtime->GetLifecycleFlushState() != WorldRuntimeLifecycleFlushState::Flushed)
            g_state->Fail("ReconcileBoundaryProbe ran before framework lifecycle scope flush.");

        g_state->PushTrace("reconcile-probe");
        g_state->reconcileProbeRan.store(true, std::memory_order_release);
        return ExecCallResult::Success;
    }

    ExecutionSourceDesc MakeSourceDesc(
        ExecToken token,
        ExecPhase phase,
        ExecLane lane,
        ExecNodeKind kind,
        ExecFn fn,
        const char* debugName,
        uint32_t flags = ExecNodeFlag_None,
        std::span<const AccessSpec> accesses = {})
    {
        ExecutionSourceDesc desc{};
        desc.token = token;
        desc.phase = phase;
        desc.lane = lane;
        desc.kind = kind;
        desc.flags = flags;
        desc.fn = fn;
        desc.debugName = debugName;
        desc.accesses = accesses;
        return desc;
    }

    void RegisterManualSource(
        ExecutionSourceRegistry& sourceRegistry,
        WorldExecutionModel& model,
        ExecPhase phase,
        ExecLane lane,
        ExecNodeKind kind,
        ExecFn fn,
        const char* debugName,
        uint32_t flags = ExecNodeFlag_None,
        std::span<const AccessSpec> accesses = {})
    {
        const ExecToken token = sourceRegistry.AllocateToken();
        const ExecutionSourceDesc desc =
            MakeSourceDesc(token, phase, lane, kind, fn, debugName, flags, accesses);

        Check(sourceRegistry.Register(desc), "manual source registration failed");

        switch (phase)
        {
        case ExecPhase::Simulate:
            model.simulateSources.push_back(token);
            break;
        case ExecPhase::Commit:
            model.commitSources.push_back(token);
            break;
        case ExecPhase::LifecycleFlush:
            model.lifecycleFlushSources.push_back(token);
            break;
        case ExecPhase::Reconcile:
            model.reconcileSources.push_back(token);
            break;
        default:
            Check(false, "unsupported manual source phase");
            break;
        }
    }

    std::vector<ExecToken> CollectModelTokens(const WorldExecutionModel& model)
    {
        std::vector<ExecToken> tokens;
        tokens.insert(tokens.end(), model.simulateSources.begin(), model.simulateSources.end());
        tokens.insert(tokens.end(), model.commitSources.begin(), model.commitSources.end());
        tokens.insert(tokens.end(), model.lifecycleFlushSources.begin(), model.lifecycleFlushSources.end());
        tokens.insert(tokens.end(), model.reconcileSources.begin(), model.reconcileSources.end());
        return tokens;
    }

    ExecToken FindTokenByName(
        const WorldExecutionModel& model,
        const ExecutionSourceRegistry& sourceRegistry,
        const char* debugName)
    {
        for (const ExecToken token : CollectModelTokens(model))
        {
            const ExecutionSourceDesc* desc = sourceRegistry.TryGet(token);
            if (desc != nullptr && desc->debugName == debugName)
                return token;
        }

        return InvalidExecToken;
    }

    ExecNodeId FindNodeByToken(const FrameTaskGraph& graph, ExecToken token)
    {
        for (const ExecNodeRecord& node : graph.nodes)
        {
            if (node.sourceToken == token)
                return node.id;
        }

        return InvalidExecNodeId;
    }

    bool HasEdge(const FrameTaskGraph& graph, ExecToken fromToken, ExecToken toToken)
    {
        const ExecNodeId fromNodeId = FindNodeByToken(graph, fromToken);
        const ExecNodeId toNodeId = FindNodeByToken(graph, toToken);
        if (!graph.IsValidNodeId(fromNodeId) || !graph.IsValidNodeId(toNodeId))
            return false;

        const ExecNodeRecord& fromNode = graph.nodes[fromNodeId];
        for (uint32_t i = 0; i < fromNode.succCount; ++i)
        {
            const uint32_t edgeIndex = fromNode.succBegin + i;
            if (edgeIndex < graph.edges.size() && graph.edges[edgeIndex] == toNodeId)
                return true;
        }

        return false;
    }

    void VerifyGraphShape(
        const BuildResult& buildResult,
        const WorldExecutionModel& model,
        const ExecutionSourceRegistry& sourceRegistry)
    {
        Check(buildResult.success, "graph build did not succeed");
        Check(!buildResult.HasError(), "graph build produced error diagnostics");

        const FrameTaskGraph& graph = buildResult.graph;
        Check(graph.scopeCount == 1, "expected exactly one execution scope");
        Check(graph.nodes.size() == 15, "expected 15 nodes: 12 simulate + 3 serial probes");
        Check(graph.simulateNodeCount == 12, "expected 12 simulate nodes");

        const ExecToken perceptionA =
            FindTokenByName(model, sourceRegistry, "PerceptionReadASystem");
        const ExecToken perceptionB =
            FindTokenByName(model, sourceRegistry, "PerceptionReadBSystem");
        const ExecToken aiCommand =
            FindTokenByName(model, sourceRegistry, "AICommandSystem");
        const ExecToken playerCommand =
            FindTokenByName(model, sourceRegistry, "PlayerCommandSystem");
        const ExecToken action =
            FindTokenByName(model, sourceRegistry, "ResolveActionStateSystem");
        const ExecToken animation =
            FindTokenByName(model, sourceRegistry, "ResolveAnimationSystem");
        const ExecToken boss =
            FindTokenByName(model, sourceRegistry, "BossPhaseSystem");
        const ExecToken spawn =
            FindTokenByName(model, sourceRegistry, "SpawnRequestSystem");
        const ExecToken orderingSource =
            FindTokenByName(model, sourceRegistry, "OrderingHintSourceSystem");
        const ExecToken orderingTarget =
            FindTokenByName(model, sourceRegistry, "OrderingHintTargetSystem");
        const ExecToken presentation =
            FindTokenByName(model, sourceRegistry, "Presentation.MainThreadOnly");

        Check(perceptionA != InvalidExecToken, "PerceptionReadASystem token not found");
        Check(perceptionB != InvalidExecToken, "PerceptionReadBSystem token not found");
        Check(aiCommand != InvalidExecToken, "AICommandSystem token not found");
        Check(playerCommand != InvalidExecToken, "PlayerCommandSystem token not found");
        Check(action != InvalidExecToken, "ResolveActionStateSystem token not found");
        Check(animation != InvalidExecToken, "ResolveAnimationSystem token not found");
        Check(boss != InvalidExecToken, "BossPhaseSystem token not found");
        Check(spawn != InvalidExecToken, "SpawnRequestSystem token not found");
        Check(orderingSource != InvalidExecToken, "OrderingHintSourceSystem token not found");
        Check(orderingTarget != InvalidExecToken, "OrderingHintTargetSystem token not found");
        Check(presentation != InvalidExecToken, "Presentation.MainThreadOnly token not found");

        Check(HasEdge(graph, aiCommand, action),
            "missing auto edge: AICommandSystem -> ResolveActionStateSystem");
        Check(HasEdge(graph, playerCommand, action),
            "missing auto edge: PlayerCommandSystem -> ResolveActionStateSystem");
        Check(HasEdge(graph, action, animation),
            "missing auto edge: ResolveActionStateSystem -> ResolveAnimationSystem");
        Check(HasEdge(graph, animation, boss),
            "missing auto edge: ResolveAnimationSystem -> BossPhaseSystem");
        Check(HasEdge(graph, boss, spawn),
            "missing auto edge: BossPhaseSystem -> SpawnRequestSystem");
        Check(HasEdge(graph, orderingSource, orderingTarget),
            "missing runsBefore edge: OrderingHintSourceSystem -> OrderingHintTargetSystem");
        Check(HasEdge(graph, animation, presentation),
            "missing mixed manual/auto edge: ResolveAnimationSystem -> Presentation.MainThreadOnly");
        Check(HasEdge(graph, boss, presentation),
            "missing mixed manual/auto edge: BossPhaseSystem -> Presentation.MainThreadOnly");
        Check(!HasEdge(graph, perceptionA, perceptionB) && !HasEdge(graph, perceptionB, perceptionA),
            "snapshot read systems should not conflict with each other");
    }

    void RunPhase1AutoSystemBridgeSmokeTest()
    {
        constexpr WorldExecutionModelKey kModelKey = 501;

        Phase1SmokeState state{};
        state.mainThreadId = std::this_thread::get_id();
        g_state = &state;

        WorldExecutionModel model{};
        model.key = kModelKey;

        WorldDef def = MakeSmokeWorldDef(kModelKey);
        WorldRuntime runtime(WorldRuntimeCreateParams{
            &def,
            &model,
            nullptr,
            nullptr
        });

        SystemManager systemManager{};
        systemManager.RegisterSystem<PerceptionReadASystem>(SystemPhase::Graph, &state);
        systemManager.RegisterSystem<PerceptionReadBSystem>(SystemPhase::Graph, &state);
        systemManager.RegisterSystem<AICommandSystem>(SystemPhase::Graph, &state);
        systemManager.RegisterSystem<PlayerCommandSystem>(SystemPhase::Graph, &state);
        systemManager.RegisterSystem<ResolveActionStateSystem>(SystemPhase::Graph, &state);
        systemManager.RegisterSystem<ResolveLocomotionSystem>(SystemPhase::Graph, &state);
        systemManager.RegisterSystem<ResolveAnimationSystem>(SystemPhase::Graph, &state);
        systemManager.RegisterSystem<BossPhaseSystem>(SystemPhase::Graph, &state);
        systemManager.RegisterSystem<SpawnRequestSystem>(SystemPhase::Graph, &state);
        systemManager.RegisterSystem<OrderingHintTargetSystem>(SystemPhase::Graph, &state);
        systemManager.RegisterSystem<OrderingHintSourceSystem>(SystemPhase::Graph, &state);

        ExecutionSourceRegistry sourceRegistry{};
        AutoSystemBridge bridge{};
        const AutoSystemBridge::BridgeResult bridgeResult = bridge.Bridge(
            SystemPhase::Graph,
            ExecPhase::Simulate,
            systemManager,
            sourceRegistry,
            runtime,
            model);

        Check(bridgeResult.success, bridgeResult.errorMessage.c_str());
        Check(bridgeResult.registeredCount == 11, "AutoSystemBridge registered unexpected system count");

        static const std::array<AccessSpec, 2> kPresentationAccesses{
            ReadImmediate(ComponentRes<AnimationStateComponent>()),
            ReadImmediate(ComponentRes<BossPhaseComponent>())
        };

        RegisterManualSource(
            sourceRegistry,
            model,
            ExecPhase::Simulate,
            ExecLane::Main,
            ExecNodeKind::StaticSystem,
            &PresentationMainThread,
            "Presentation.MainThreadOnly",
            ExecNodeFlag_MainThreadOnly,
            kPresentationAccesses);

        RegisterManualSource(
            sourceRegistry,
            model,
            ExecPhase::Commit,
            ExecLane::Serial,
            ExecNodeKind::PostCommitFinalize,
            &CommitBoundaryProbe,
            "CommitBoundaryProbe");

        RegisterManualSource(
            sourceRegistry,
            model,
            ExecPhase::LifecycleFlush,
            ExecLane::Serial,
            ExecNodeKind::LifecycleFlush,
            &LifecycleBoundaryProbe,
            "LifecycleBoundaryProbe");

        RegisterManualSource(
            sourceRegistry,
            model,
            ExecPhase::Reconcile,
            ExecLane::Serial,
            ExecNodeKind::Reconcile,
            &ReconcileBoundaryProbe,
            "ReconcileBoundaryProbe");

        WorldExecutionModelRegistry modelRegistry{};
        Check(modelRegistry.Register(model, sourceRegistry),
            "WorldExecutionModelRegistry rejected the bridged model");

        Check(runtime.Initialize(), "WorldRuntime initialize failed");
        runtime.ClearLifecycleOutbox();
        Check(runtime.BeginFrame(1, 100.0, 0.016), "WorldRuntime BeginFrame failed");

        WorldFrameSelectionSet selections{};
        const WorldId worldId = WorldId::Create(77, 1);
        selections.selections.push_back(WorldFrameSelection{
            0,
            worldId.GetRaw(),
            kModelKey
        });

        ExecutionGraphBuilder graphBuilder{};
        ExecutionGraphBuildPolicy buildPolicy{};
        FrameBuildContext buildContext{};
        buildContext.frameSelectionSet = &selections;
        buildContext.executionModelRegistry = &modelRegistry;
        buildContext.executionSourceRegistry = &sourceRegistry;
        buildContext.buildPolicy = &buildPolicy;

        const BuildResult buildResult = graphBuilder.Build(buildContext);
        VerifyGraphShape(buildResult, model, sourceRegistry);

        std::vector<WorldRuntime*> runtimeByScope{ &runtime };
        std::vector<WorldId> worldIdByScope{ worldId };
        std::vector<ExecNodeRuntime> nodeRuntime(buildResult.graph.nodes.size());
        std::vector<ExecScopeRuntime> scopeRuntime(buildResult.graph.scopeCount);

        ExecutionOps ops = MakeDummyValidOps();
        FrameExecContext frameExec{};
        frameExec.graph = &buildResult.graph;
        frameExec.ops = &ops;
        frameExec.runtimeByScope = runtimeByScope;
        frameExec.worldIdByScope = worldIdByScope;

        ExecRuntimeState execRuntime{};
        execRuntime.BindViews(
            std::span<ExecNodeRuntime>(nodeRuntime.data(), nodeRuntime.size()),
            std::span<ExecScopeRuntime>(scopeRuntime.data(), scopeRuntime.size()),
            buildResult.graph.simulateNodeCount);

        TaskExecutor executor(4);
        Check(executor.IsInitialized(), "TaskExecutor did not initialize");
        Check(executor.ExecuteFrame(frameExec, execRuntime, sourceRegistry),
            "TaskExecutor ExecuteFrame returned false");

        CheckNoRecordedFailures(state);

        Check(state.perceptionA.load(std::memory_order_acquire), "PerceptionReadASystem did not run");
        Check(state.perceptionB.load(std::memory_order_acquire), "PerceptionReadBSystem did not run");
        Check(state.aiCommand.load(std::memory_order_acquire), "AICommandSystem did not run");
        Check(state.playerCommand.load(std::memory_order_acquire), "PlayerCommandSystem did not run");
        Check(state.actionResolved.load(std::memory_order_acquire), "ResolveActionStateSystem did not run");
        Check(state.locomotionResolved.load(std::memory_order_acquire), "ResolveLocomotionSystem did not run");
        Check(state.animationResolved.load(std::memory_order_acquire), "ResolveAnimationSystem did not run");
        Check(state.bossPhaseResolved.load(std::memory_order_acquire), "BossPhaseSystem did not run");
        Check(state.spawnRequested.load(std::memory_order_acquire), "SpawnRequestSystem did not run");
        Check(state.orderingBeforeRan.load(std::memory_order_acquire), "OrderingHintSourceSystem did not run");
        Check(state.orderingAfterRan.load(std::memory_order_acquire), "OrderingHintTargetSystem did not run");
        Check(state.presentationRan.load(std::memory_order_acquire), "PresentationMainThread did not run");
        Check(state.presentationOnMainThread.load(std::memory_order_acquire),
            "MainThreadOnly node did not execute on main thread");
        Check(state.commitProbeRan.load(std::memory_order_acquire), "CommitBoundaryProbe did not run");
        Check(state.lifecycleProbeRan.load(std::memory_order_acquire), "LifecycleBoundaryProbe did not run");
        Check(state.reconcileProbeRan.load(std::memory_order_acquire), "ReconcileBoundaryProbe did not run");

        Check(state.maxSnapshotReaders.load(std::memory_order_acquire) >= 2,
            "snapshot read-only systems did not overlap; expected observable parallelism");
        Check(!runtime.IsFaulted(), "WorldRuntime faulted during Phase1 smoke");
        Check(runtime.GetCommitState() == WorldRuntimeCommitState::CommitSucceeded,
            "framework commit scope did not flush successfully");
        Check(runtime.GetLifecycleFlushState() == WorldRuntimeLifecycleFlushState::Flushed,
            "framework lifecycle scope did not flush successfully");
        Check(runtime.LifecycleOutbox().size() == 1,
            "expected one lifecycle event from deferred entity materialization");

        std::cout
            << "[PASS] Phase1 AutoSystemBridge smoke"
            << " nodes=" << buildResult.graph.nodes.size()
            << " edges=" << buildResult.graph.edges.size()
            << " maxSnapshotReaders=" << state.maxSnapshotReaders.load()
            << " lifecycleOutbox=" << runtime.LifecycleOutbox().size()
            << "\n";
    }
}

void RunTaskExecutorPerfDiagnostics();

int main()
{
    try
    {
        RunPhase1AutoSystemBridgeSmokeTest();
        RunTaskExecutorPerfDiagnostics();
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "[FAIL] WITH_Server_Framework_Test: " << ex.what() << "\n";
        return 1;
    }
}
