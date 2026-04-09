#include "pch.h"
#include "WorldScheduler.h"

#include <algorithm>

#include "ExecutionGraphBuildPolicy.h"
#include "ExecutionGraphBuilder.h"
#include "ExecutionSourceTypes.h"
#include "ExecutionOps.h"
#include "TaskExecutor.h"
#include "WorldExecutionModelTypes.h"
#include "WorldInstance.h"
#include "WorldManager.h"
#include "WorldRegistry.h"

namespace
{
    static WorldId WorldIdFromBinding(const WorldBinding binding) noexcept
    {
        if (binding == InvalidWorldBinding)
            return WorldId::Invalid();

        const uint32_t id = static_cast<uint32_t>(binding & 0xFFFFFFFFull);
        const uint32_t gen = static_cast<uint32_t>((binding >> 32) & 0xFFFFFFFFull);
        return WorldId::Create(id, gen);
    }
}

WorldScheduler::WorldScheduler(
    WorldManager& worldManager,
    WorldRegistry& worldRegistry,
    WorldExecutionModelRegistry& executionModelRegistry,
    ExecutionSourceRegistry& executionSourceRegistry,
    ExecutionGraphBuilder& graphBuilder,
    const ExecutionGraphBuildPolicy& buildPolicy,
    TaskExecutor& executor,
    ExecutionOps& executionOps,
    WorldSchedulerConfig config)
    : _worldManager(worldManager)
    , _worldRegistry(worldRegistry)
    , _executionModelRegistry(executionModelRegistry)
    , _executionSourceRegistry(executionSourceRegistry)
    , _graphBuilder(graphBuilder)
    , _buildPolicy(buildPolicy)
    , _executor(executor)
    , _executionOps(executionOps)
    , _config(config)
{
}

bool WorldScheduler::RunFrame(
    const WorldSchedulerFrameParams& params,
    WorldSchedulerFrameResult& outResult)
{
    outResult = {};
    _scratch.Reset();

    if (!_executionOps.IsValid())
    {
        outResult.failureReason = WorldSchedulerFailureReason::InternalInvariant;
        return false;
    }

    if (!BuildSelectionSet(params, _scratch.selections))
    {
        outResult.failureReason = WorldSchedulerFailureReason::InternalInvariant;
        return false;
    }

    outResult.selectedWorldCount = _scratch.selections.GetCount();
    if (_scratch.selections.IsEmpty())
    {
        outResult.success = true;
        return true;
    }

    if (!ResolveSelectedRuntimes(
        _scratch.selections,
        _scratch.runtimeByScope,
        _scratch.worldIdByScope))
    {
        outResult.failureReason = WorldSchedulerFailureReason::RuntimeResolveFailed;
        return false;
    }

    if (!BuildFrameGraph(_scratch.selections, _scratch.buildResult))
    {
        outResult.failureReason = WorldSchedulerFailureReason::GraphBuildFailed;
        return false;
    }
    outResult.graphBuilt = true;

    if (!_scratch.buildResult.success)
    {
        outResult.failureReason = WorldSchedulerFailureReason::GraphBuildFailed;
        return false;
    }

    if (!BeginSelectedFrames(params, std::span<WorldRuntime*>(
        _scratch.runtimeByScope.data(),
        _scratch.runtimeByScope.size())))
    {
        outResult.failureReason = WorldSchedulerFailureReason::BeginFrameFailed;
        return false;
    }

    if (!PrepareExecutionContexts(
        _scratch.buildResult.graph,
        _scratch.runtimeByScope,
        _scratch.worldIdByScope,
        _scratch.frameExec,
        _scratch.execRuntime))
    {
        outResult.failureReason = WorldSchedulerFailureReason::InternalInvariant;
        return false;
    }

    if (!_executor.ExecuteFrame(
        _scratch.frameExec,
        _scratch.execRuntime,
        _executionSourceRegistry))
    {
        outResult.failureReason = WorldSchedulerFailureReason::ExecutorFailed;
        return false;
    }

    outResult.executed = true;
    outResult.success = true;
    return true;
}

const WorldFrameSelectionSet& WorldScheduler::GetLastSelectionSet() const noexcept
{
    return _scratch.selections;
}

const BuildResult& WorldScheduler::GetLastBuildResult() const noexcept
{
    return _scratch.buildResult;
}

void WorldScheduler::Clear() noexcept
{
    _scratch.Reset();
}

void WorldScheduler::FrameScratch::Reset() noexcept
{
    selections.Clear();

    buildResult = {};

    runtimeByScope.clear();
    worldIdByScope.clear();

    nodeBacking.reset();
    scopeBacking.reset();

    frameExec.Clear();
    execRuntime.ClearViews();
}

bool WorldScheduler::BuildSelectionSet(
    const WorldSchedulerFrameParams& params,
    WorldFrameSelectionSet& outSelections)
{
    (void)params;

    outSelections.Clear();

    const std::span<const WorldId> runnableWorldIds = _worldManager.GetRunnableWorldIds();

    uint32_t nextScopeId = 0;
    for (const WorldId worldId : runnableWorldIds)
    {
        const WorldInstanceRecord* record = _worldManager.FindRecord(worldId);
        if (record == nullptr || !record->IsRunnable())
            continue;

        WorldInstance* world = _worldRegistry.FindWorld(worldId);
        if (world == nullptr)
            return false;

        if (world->IsFaulted())
            continue;

        const WorldExecutionModel& model = world->GetExecutionModel();
        if (!model.IsValid())
            return false;

        WorldFrameSelection selection{};
        selection.scopeId = nextScopeId++;
        selection.worldBinding = worldId.GetRaw();
        selection.modelKey = model.key;
        outSelections.selections.push_back(selection);

        if (_config.maxSelectedWorldsPerFrame != 0 &&
            outSelections.GetCount() >= _config.maxSelectedWorldsPerFrame)
        {
            break;
        }
    }

    return true;
}

bool WorldScheduler::ResolveSelectedRuntimes(
    const WorldFrameSelectionSet& selections,
    std::vector<WorldRuntime*>& outRuntimeByScope,
    std::vector<WorldId>& outWorldIdByScope)
{
    outRuntimeByScope.clear();
    outRuntimeByScope.resize(selections.GetCount(), nullptr);
    outWorldIdByScope.clear();
    outWorldIdByScope.resize(selections.GetCount(), WorldId::Invalid());

    for (const WorldFrameSelection& selection : selections.selections)
    {
        if (!selection.IsValid())
            return false;

        if (selection.scopeId >= outRuntimeByScope.size())
            return false;

        const WorldId worldId = WorldIdFromBinding(selection.worldBinding);
        if (!worldId.IsValid())
            return false;

        WorldInstance* world = _worldRegistry.FindWorld(worldId);
        if (world == nullptr)
            return false;

        outRuntimeByScope[selection.scopeId] = &world->GetRuntime();
        outWorldIdByScope[selection.scopeId] = worldId;
    }

    return true;
}

bool WorldScheduler::BeginSelectedFrames(
    const WorldSchedulerFrameParams& params,
    std::span<WorldRuntime*> runtimeByScope)
{
    for (WorldRuntime* runtime : runtimeByScope)
    {
        if (runtime == nullptr)
            return false;

        if (!runtime->BeginFrame(params.frameIndex, params.nowSec, params.dtSec))
            return false;
    }

    return true;
}

bool WorldScheduler::BuildFrameGraph(
    const WorldFrameSelectionSet& selections,
    BuildResult& outBuildResult)
{
    FrameBuildContext buildContext{};
    buildContext.frameSelectionSet = &selections;
    buildContext.executionModelRegistry = &_executionModelRegistry;
    buildContext.executionSourceRegistry = &_executionSourceRegistry;
    buildContext.buildPolicy = &_buildPolicy;

    outBuildResult = _graphBuilder.Build(buildContext);
    return !outBuildResult.HasError();
}

bool WorldScheduler::PrepareExecutionContexts(
    const FrameTaskGraph& graph,
    std::vector<WorldRuntime*>& runtimeByScope,
    std::vector<WorldId>& worldIdByScope,
    FrameExecContext& outFrameExec,
    ExecRuntimeState& outExecRuntime)
{
    if (!_executionOps.IsValid())
        return false;

    if (graph.scopeCount != runtimeByScope.size())
        return false;

    if (graph.scopeCount != worldIdByScope.size())
        return false;

    if (!graph.nodes.empty())
        _scratch.nodeBacking = std::make_unique<ExecNodeRuntime[]>(graph.nodes.size());
    else
        _scratch.nodeBacking.reset();

    if (graph.scopeCount > 0)
        _scratch.scopeBacking = std::make_unique<ExecScopeRuntime[]>(graph.scopeCount);
    else
        _scratch.scopeBacking.reset();

    outFrameExec.Clear();
    outFrameExec.graph = &graph;
    outFrameExec.ops = &_executionOps;
    outFrameExec.runtimeByScope = std::span<WorldRuntime*>(
        runtimeByScope.data(),
        runtimeByScope.size());
    outFrameExec.worldIdByScope = std::span<const WorldId>(
        worldIdByScope.data(),
        worldIdByScope.size());

    outExecRuntime.BindViews(
        std::span<ExecNodeRuntime>(
            _scratch.nodeBacking.get(),
            graph.nodes.size()),
        std::span<ExecScopeRuntime>(
            _scratch.scopeBacking.get(),
            graph.scopeCount));

    return true;
}
