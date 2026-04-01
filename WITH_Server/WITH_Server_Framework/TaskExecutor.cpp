#include "pch.h"
#include "TaskExecutor.h"

#include <algorithm>
#include <cassert>
#include <thread>

TaskExecutor::TaskExecutor(uint32_t workerCount)
{
    if (workerCount > 0)
        (void)Initialize(workerCount);
}

TaskExecutor::~TaskExecutor()
{
    Shutdown();
}

bool TaskExecutor::Initialize(uint32_t workerCount)
{
    bool expected{ false };
    if (!_initialized.compare_exchange_strong(expected, true))
    {
        return true;
    }

    try
    {
        if (!_pool.Start(workerCount, &TaskExecutor::WorkerPumpEntry, this))
        {
            _initialized.store(false);
            return false;
        }
    }
    catch (...)
    {
        _initialized.store(false);
        throw;
    }

    return true;
}

void TaskExecutor::Shutdown() noexcept
{
    bool expected{ true };
    if (!_initialized.compare_exchange_strong(expected, false))
    {
        return;
    }

    _frameBound.store(false);
    _pool.Stop();

    UnbindFrame();
}

bool TaskExecutor::ExecuteFrame(
    FrameExecContext& frameCtx,
    ExecRuntimeState& runtime,
    const ExecutionSourceRegistry& sourceRegistry)
{
    if (!IsInitialized())
        return false;

    if (!ValidateFrameInputs(frameCtx, runtime))
        return false;

    BindFrame(frameCtx, runtime, sourceRegistry);
    InitializeRuntimeForFrame();
    
    SeedInitialSimulateNodes();
    NotifyAllWorkers();
    WaitForSimulateDone();

    const FrameTaskGraph& graph = Graph();

    RunSerialPhase(graph.commitPlan, ExecPhase::Commit);
    RunScopeSerialPhase(graph.commitScopePlan, ExecPhase::Commit);
    FinalizeScopeClosures();

    RunSerialPhase(graph.lifecycleFlushPlan, ExecPhase::LifecycleFlush);
    RunScopeSerialPhase(graph.lifecycleFlushScopePlan, ExecPhase::LifecycleFlush);
    FinalizeScopeClosures();

    RunSerialPhase(graph.reconcilePlan, ExecPhase::Reconcile);
    RunScopeSerialPhase(graph.reconcileScopePlan, ExecPhase::Reconcile);
    FinalizeScopeClosures();

    UnbindFrame();
    return true;
}

bool TaskExecutor::WorkerPumpEntry(void* ctx)
{
    if (ctx == nullptr)
        return false;

    return static_cast<TaskExecutor*>(ctx)->WorkerPump();
}

bool TaskExecutor::WorkerPump()
{
    if (!_frameBound.load())
        return false;

    ExecNodeId nodeId = InvalidExecNodeId;
    if (!TryDequeueReadyNode(nodeId))
        return false;

    ExecuteNode(nodeId);
    return true;
}

bool TaskExecutor::ValidateFrameInputs(
    const FrameExecContext& frameCtx,
    const ExecRuntimeState& runtime) const noexcept
{
    if (!frameCtx.IsValid())
        return false;

    if (frameCtx.graph == nullptr)
        return false;

    const FrameTaskGraph& graph = *frameCtx.graph;

    if (runtime.nodes.size() < graph.nodes.size())
        return false;

    if (runtime.scopes.size() < graph.scopeCount)
        return false;

    return true;
}

void TaskExecutor::BindFrame(
    FrameExecContext& frameCtx,
    ExecRuntimeState& runtime,
    const ExecutionSourceRegistry& sourceRegistry) noexcept
{
    _binding.frame = &frameCtx;
    _binding.runtime = &runtime;
    _binding.sources = &sourceRegistry;

    {
        std::lock_guard lock{ _readyMtx };
        _readyQueue.clear();
    }

    _frameBound.store(true);
}

void TaskExecutor::UnbindFrame() noexcept
{
    _frameBound.store(false);

    {
        std::lock_guard lock{ _readyMtx };
        _readyQueue.clear();
    }

    _binding = {};
}

void TaskExecutor::InitializeRuntimeForFrame()
{
    const FrameTaskGraph& graph = Graph();
    ExecRuntimeState& runtime = Runtime();

    std::vector<uint32_t> nodeDepCounts(graph.nodes.size(), 0);
    std::vector<uint32_t> scopeNodeCounts(graph.scopeCount, 0);

    for (ExecNodeId nodeId = 0;
        nodeId < static_cast<ExecNodeId>(graph.nodes.size());
        ++nodeId)
    {
        const ExecNodeRecord& node = graph.nodes[nodeId];

        if (node.phase == ExecPhase::Simulate)
            nodeDepCounts[nodeId] = node.predCount;
        else
            nodeDepCounts[nodeId] = 0;

        if (graph.IsValidScopeId(node.scopeId))
            ++scopeNodeCounts[node.scopeId];
    }

    runtime.ResetAll(nodeDepCounts, scopeNodeCounts, graph.simulateNodeCount);
}

void TaskExecutor::SeedInitialSimulateNodes()
{
    const FrameTaskGraph& graph = Graph();
    ExecRuntimeState& runtime = Runtime();

    for (ExecNodeId nodeId = 0;
        nodeId < static_cast<ExecNodeId>(graph.nodes.size());
        ++nodeId)
    {
        const ExecNodeRecord& node = graph.nodes[nodeId];
        if (node.phase != ExecPhase::Simulate)
            continue;

        ExecNodeRuntime* nodeRt = runtime.TryGetNode(nodeId);
        ExecScopeRuntime* scopeRt = runtime.TryGetScope(node.scopeId);
        if (nodeRt == nullptr || scopeRt == nullptr)
            continue;

        if (nodeRt->remainingDeps.load() != 0)
            continue;

        const ExecScopePhase scopePhase = scopeRt->phase.load();

        if (IsExecutableScopePhase(scopePhase))
        {
            if (TryTransitionNode(*nodeRt, ExecNodeState::NotReady, ExecNodeState::Ready))
            {
                DispatchNode(nodeId);
            }
        }
        else
        {
            const ExecNodeState terminal = SelectCancelTerminalState(node);
            if (TryTransitionNode(*nodeRt, ExecNodeState::NotReady, terminal))
            {
                CompleteNodeTerminal(nodeId, terminal);
            }
        }
    }

    if (graph.simulateNodeCount == 0)
    {
        runtime.signals.simulatePhaseDone.store(true);
        NotifyProgress();
    }
}

void TaskExecutor::RunSerialPhase(
    const ExecRange& range,
    ExecPhase expectedPhase)
{
    const FrameTaskGraph& graph = Graph();
    ExecRuntimeState& runtime = Runtime();

    if (range.IsEmpty())
        return;

    assert(graph.IsValidSerialRange(range));

    for (uint32_t i = 0; i < range.count; ++i)
    {
        const ExecNodeId nodeId =
            graph.serialExecutionOrder[range.begin + i];
        assert(graph.IsValidNodeId(nodeId));

        const ExecNodeRecord& node = graph.nodes[nodeId];
        assert(node.phase == expectedPhase);

        ExecNodeRuntime* nodeRt = runtime.TryGetNode(nodeId);
        ExecScopeRuntime* scopeRt = runtime.TryGetScope(node.scopeId);
        assert(nodeRt != nullptr);
        assert(scopeRt != nullptr);

        const ExecScopePhase scopePhase = scopeRt->phase.load();

        if (!IsExecutableScopePhase(scopePhase))
        {
            const ExecNodeState terminal = SelectCancelTerminalState(node);
            const bool ok =
                TryTransitionNode(*nodeRt, ExecNodeState::NotReady, terminal);
            assert(ok);

            CompleteNodeTerminal(nodeId, terminal);
            continue;
        }

        const bool entered =
            TryTransitionNode(*nodeRt, ExecNodeState::NotReady, ExecNodeState::Running);
        assert(entered);

        const ExecCallResult callResult = InvokeNode(node, nodeId);

        if (callResult == ExecCallResult::Success)
        {
            const bool ok = 
                TryTransitionNode(*nodeRt, ExecNodeState::Running, ExecNodeState::Succeeded);
            assert(ok);
            CompleteNodeTerminal(nodeId, ExecNodeState::Succeeded);
        }
        else
        {
            const bool ok = 
                TryTransitionNode(*nodeRt, ExecNodeState::Running, ExecNodeState::Failed);
            assert(ok);

            MarkScopeFailedAndCancelRequested(node.scopeId);
            CompleteNodeTerminal(nodeId, ExecNodeState::Failed);
        }
    }
}

void TaskExecutor::RunScopeSerialPhase(
    const ExecRange& range,
    ExecPhase expectedPhase)
{
    const FrameTaskGraph& graph = Graph();
    FrameExecContext& frame = Frame();

    if (range.IsEmpty())
        return;

    assert(graph.IsValidSerialScopeRange(range));
    assert(frame.ops != nullptr);

    for (uint32_t i = 0; i < range.count; ++i)
    {
        const ExecScopeId scopeId =
            graph.serialScopeOrder[range.begin + i];

        assert(frame.IsValidScopeId(scopeId));

        switch (expectedPhase) {
        case ExecPhase::Commit:
            frame.ops->CommitScope(scopeId, frame.runtimeByScope);
            break;

        case ExecPhase::LifecycleFlush:
            frame.ops->FlushLifecycle(scopeId, frame.runtimeByScope);
            break;

        case ExecPhase::Reconcile:
            frame.ops->ReconcileScope(scopeId, frame.runtimeByScope);
            break;

        default:
            assert(false);
            break;
        }
    }
}

void TaskExecutor::FinalizeScopeClosures() noexcept
{
    ExecRuntimeState& runtime = Runtime();

    for (ExecScopeId scopeId = 0;
        scopeId < static_cast<ExecScopeId>(runtime.scopes.size());
        ++scopeId)
    {
        ExecScopeRuntime* scopeRt = runtime.TryGetScope(scopeId);
        if (scopeRt == nullptr)
            continue;

        if (!scopeRt->closeCandidate.load())
            continue;

        if (scopeRt->remainingNodes.load() != 0)
            continue;

        ExecScopePhase current = scopeRt->phase.load();

        while (current != ExecScopePhase::Closed)
        {
            if (scopeRt->phase.compare_exchange_strong(
                current, ExecScopePhase::Closed))
            {
                break;
            }
        }
    }
}

void TaskExecutor::WaitForSimulateDone()
{
    ExecRuntimeState& runtime = Runtime();

    std::unique_lock lock{ _progressMtx };
    _progressCv.wait(lock, 
        [&]()
        {
            return runtime.signals.simulatePhaseDone.load();
        });
}

bool TaskExecutor::TryDequeueReadyNode(ExecNodeId& outNodeId)
{
    std::lock_guard lock{ _readyMtx };

    if (_readyQueue.empty())
        return false;

    outNodeId = _readyQueue.front();
    _readyQueue.pop_front();
    return true;
}

void TaskExecutor::EnqueueReadyNode(ExecNodeId nodeId)
{
    {
        std::lock_guard lock{ _readyMtx };
        _readyQueue.push_back(nodeId);
    }

    NotifyWork();
}

void TaskExecutor::DispatchNode(ExecNodeId nodeId)
{
    ExecRuntimeState& runtime = Runtime();
    ExecNodeRuntime* nodeRt = runtime.TryGetNode(nodeId);
    if (nodeRt == nullptr)
        return;

    const bool ok = 
        TryTransitionNode(*nodeRt, ExecNodeState::Ready, ExecNodeState::Queued);

    if (!ok)
        return;

    EnqueueReadyNode(nodeId);
}

void TaskExecutor::ExecuteNode(ExecNodeId nodeId)
{
    const FrameTaskGraph& graph = Graph();
    ExecRuntimeState& runtime = Runtime();

    assert(graph.IsValidNodeId(nodeId));

    const ExecNodeRecord& node = graph.nodes[nodeId];
    ExecNodeRuntime* nodeRt = runtime.TryGetNode(nodeId);
    ExecScopeRuntime* scopeRt = runtime.TryGetScope(node.scopeId);

    assert(nodeRt != nullptr);
    assert(scopeRt != nullptr);

    const ExecScopePhase scopePhase = scopeRt->phase.load();

    if (!IsExecutableScopePhase(scopePhase))
    {
        const ExecNodeState terminal = SelectCancelTerminalState(node);

        const bool ok = 
            TryTransitionNode(*nodeRt, ExecNodeState::Queued, terminal);
        assert(ok);

        CompleteNodeTerminal(nodeId, terminal);
        return;
    }

    const bool canRun = 
        TryTransitionNode(*nodeRt, ExecNodeState::Queued, ExecNodeState::Running);
    assert(canRun);

    const ExecCallResult callResult = InvokeNode(node, nodeId);

    if (callResult == ExecCallResult::Success)
    {
        const bool ok = 
            TryTransitionNode(*nodeRt, ExecNodeState::Running, ExecNodeState::Succeeded);
        assert(ok);

        CompleteNodeTerminal(nodeId, ExecNodeState::Succeeded);
        ResolveSuccessors(nodeId);
    }
    else
    {
        const bool ok = 
            TryTransitionNode(*nodeRt, ExecNodeState::Running, ExecNodeState::Failed);
        assert(ok);

        MarkScopeFailedAndCancelRequested(node.scopeId);
        CompleteNodeTerminal(nodeId, ExecNodeState::Failed);
        ResolveSuccessors(nodeId);
    }
}

ExecCallResult TaskExecutor::InvokeNode(
    const ExecNodeRecord& node,
    ExecNodeId nodeId) const
{
    const ExecutionSourceDesc* desc =
        Sources().TryGet(node.sourceToken);

    if (desc == nullptr)
        return ExecCallResult::Failed;

    if (desc->fn == nullptr)
        return ExecCallResult::Failed;

    NodeScratch scratch{};
    NodeExecContext ctx{};
    ctx.frame = _binding.frame;
    ctx.nodeId = nodeId;
    ctx.scopeId = node.scopeId;
    ctx.sourceToken = node.sourceToken;
    ctx.scratch = &scratch;

    try
    {
        return desc->fn(ctx);
    }
    catch (...)
    {
        if (HasAnyNodeFlag(node.flags, ExecNodeFlag_NoThrow))
            std::terminate();

        return ExecCallResult::Failed;
    }
}

void TaskExecutor::ResolveSuccessors(ExecNodeId completedNodeId)
{
    const FrameTaskGraph& graph = Graph();
    ExecRuntimeState& runtime = Runtime();

    const ExecNodeRecord& node = graph.nodes[completedNodeId];
    if (!graph.IsValidSuccRange(node))
        return;

    for (uint32_t i = 0; i < node.succCount; ++i)
    {
        const uint32_t edgeIndex = node.succBegin + i;
        if (edgeIndex >= graph.edges.size())
            break;

        const ExecNodeId succId = graph.edges[edgeIndex];
        if (!graph.IsValidNodeId(succId))
            continue;

        const ExecNodeRecord& succNode = graph.nodes[succId];
        if (succNode.phase != ExecPhase::Simulate)
            continue;

        ExecNodeRuntime* succRt = runtime.TryGetNode(succId);
        ExecScopeRuntime* succScopeRt = runtime.TryGetScope(succNode.scopeId);
        if (succRt == nullptr || succScopeRt == nullptr)
            continue;

        const uint32_t prev =
            succRt->remainingDeps.fetch_sub(1);

        assert(prev > 0);

        if (prev != 1)
            continue;

        const ExecScopePhase scopePhase =
            succScopeRt->phase.load();

        if (IsExecutableScopePhase(scopePhase))
        {
            const bool ok = 
                TryTransitionNode(*succRt, ExecNodeState::NotReady, ExecNodeState::Ready);
            assert(ok);
            DispatchNode(succId);
        }
        else
        {
            const ExecNodeState terminal =
                SelectCancelTerminalState(succNode);

            const bool ok = 
                TryTransitionNode(*succRt, ExecNodeState::NotReady, terminal);
            assert(ok);
            CompleteNodeTerminal(succId, terminal);
        }
    }
}

void TaskExecutor::CompleteNodeTerminal(
    ExecNodeId nodeId,
    ExecNodeState terminalState) noexcept
{
    assert(IsTerminalNodeState(terminalState));

    const FrameTaskGraph& graph = Graph();
    ExecRuntimeState& runtime = Runtime();
    const ExecNodeRecord& node = graph.nodes[nodeId];

    ExecScopeRuntime* scopeRt = runtime.TryGetScope(node.scopeId);
    assert(scopeRt != nullptr);

    const uint32_t prevScope =
        scopeRt->remainingNodes.fetch_sub(1);
    assert(prevScope > 0);

    if (prevScope == 1)
    {
        scopeRt->closeCandidate.store(true);
    }

    if (node.phase == ExecPhase::Simulate)
    {
        const uint32_t prevSim =
            runtime.signals.remainingSimulateNodes.fetch_sub(1);
        assert(prevSim > 0);

        if (prevSim == 1)
        {
            runtime.signals.simulatePhaseDone.store(true);
        }
    }

    NotifyProgress();
}

void TaskExecutor::MarkScopeFailedAndCancelRequested(
    ExecScopeId scopeId) noexcept
{
    ExecRuntimeState& runtime = Runtime();
    ExecScopeRuntime* scopeRt = runtime.TryGetScope(scopeId);
    if (scopeRt == nullptr)
        return;

    scopeRt->flags.fetch_or(
        static_cast<uint8_t>(ExecScopeFlag_HasFailure) |
        static_cast<uint8_t>(ExecScopeFlag_WasCanceled));

    ExecScopePhase expected{ ExecScopePhase::Open };
    scopeRt->phase.compare_exchange_strong(
        expected, ExecScopePhase::CancelRequested);
}

void TaskExecutor::MarkScopeCanceled(
    ExecScopeId scopeId) noexcept
{
    ExecRuntimeState& runtime = Runtime();
    ExecScopeRuntime* scopeRt = runtime.TryGetScope(scopeId);
    if (scopeRt == nullptr)
        return;

    scopeRt->flags.fetch_or(
        static_cast<uint8_t>(ExecScopeFlag_WasCanceled));

    ExecScopePhase expected{ ExecScopePhase::Open };
    scopeRt->phase.compare_exchange_strong(
        expected, ExecScopePhase::CancelRequested);
}

ExecNodeState TaskExecutor::SelectCancelTerminalState(
    const ExecNodeRecord& node) const noexcept
{
    return HasAnyNodeFlag(node.flags, ExecNodeFlag_AllowSkipOnCancel)
        ? ExecNodeState::Skipped
        : ExecNodeState::Canceled;
}

bool TaskExecutor::TryTransitionNode(
    ExecNodeRuntime& nodeRt,
    ExecNodeState expected,
    ExecNodeState desired) const noexcept
{
    return nodeRt.state.compare_exchange_strong(expected, desired);
}

void TaskExecutor::NotifyWork() noexcept
{
    _pool.WakeOne();
}

void TaskExecutor::NotifyAllWorkers() noexcept
{
    _pool.WakeAll();
}

void TaskExecutor::NotifyProgress() noexcept
{
    std::lock_guard lock{ _progressMtx };
    _progressCv.notify_all();
}
