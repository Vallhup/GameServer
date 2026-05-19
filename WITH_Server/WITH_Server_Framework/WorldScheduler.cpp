#include "pch.h"
#include "WorldScheduler.h"

#include <algorithm>
#include <utility>

#include "DynamicTaskTypes.h"
#include "DynamicTaskScheduler.h"
#include "ExecutionGraphBuildPolicy.h"
#include "ExecutionGraphBuilder.h"
#include "ExecutionSourceTypes.h"
#include "ExecutionOps.h"
#include "TaskExecutor.h"
#include "WorldExecutionModelTypes.h"
#include "WorldInstance.h"
#include "WorldManager.h"
#include "WorldRegistry.h"
#include "WorldRuntime.h"

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
    WorldSchedulerConfig config,
    DynamicTaskTypeRegistry* dynamicTaskTypeRegistry,
    DynamicTaskScheduler* dynamicTaskScheduler)
    : _worldManager(worldManager)
    , _worldRegistry(worldRegistry)
    , _executionModelRegistry(executionModelRegistry)
    , _executionSourceRegistry(executionSourceRegistry)
    , _graphBuilder(graphBuilder)
    , _buildPolicy(buildPolicy)
    , _executor(executor)
    , _executionOps(executionOps)
    , _dynamicTaskTypeRegistry(dynamicTaskTypeRegistry)
    , _dynamicTaskScheduler(dynamicTaskScheduler)
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

    // -----------------------------------------------------------------------
    // Dynamic Task freeze boundary
    // 프레임 N 실행에서 누적된 pending 요청을 동결한다.
    // 이 시점부터 _scratch.dynamicBatch는 변경되지 않는다.
    // -----------------------------------------------------------------------
    if (_dynamicTaskTypeRegistry != nullptr)
    {
        FreezeDynamicTaskRequests(
            std::span<WorldRuntime*>(_scratch.runtimeByScope.data(), _scratch.runtimeByScope.size()),
            std::span<const WorldId>(_scratch.worldIdByScope.data(), _scratch.worldIdByScope.size()),
            params.frameIndex,
            _scratch.dynamicBatch);
    }

    if (!BuildFrameGraph(_scratch.selections, _scratch.dynamicBatch, _scratch.buildResult))
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
        _scratch.buildResult.dynamicTaskFrameTable,
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

void WorldScheduler::SetDynamicTaskScopeResolver(
    IDynamicTaskScopeResolver* resolver) noexcept
{
    _dynamicTaskScopeResolver = resolver;
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

    dynamicBatch.Clear();

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
    const DynamicTaskFrozenBatch& dynamicBatch,
    BuildResult& outBuildResult)
{
    FrameBuildContext buildContext{};
    buildContext.frameSelectionSet       = &selections;
    buildContext.executionModelRegistry  = &_executionModelRegistry;
    buildContext.executionSourceRegistry = &_executionSourceRegistry;
    buildContext.buildPolicy             = &_buildPolicy;

    // Dynamic Task 확장 — registry가 있을 때만 batch를 전달한다.
    if (_dynamicTaskTypeRegistry != nullptr && !dynamicBatch.IsEmpty())
    {
        buildContext.dynamicBatch              = &dynamicBatch;
        buildContext.dynamicTaskTypeRegistry   = _dynamicTaskTypeRegistry;
    }

    outBuildResult = _graphBuilder.Build(buildContext);
    return !outBuildResult.HasError();
}

bool WorldScheduler::PrepareExecutionContexts(
    const FrameTaskGraph& graph,
    DynamicTaskFrameTable& dynamicTaskFrameTable,
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
    outFrameExec.ops   = &_executionOps;
    outFrameExec.runtimeByScope = std::span<WorldRuntime*>(runtimeByScope.data(), runtimeByScope.size());
    outFrameExec.worldIdByScope = std::span<const WorldId>(worldIdByScope.data(), worldIdByScope.size());

    // dynamicTaskFrameTable은 buildResult 안에 있으며 FrameScratch와 수명이 같다.
    if (!dynamicTaskFrameTable.IsEmpty())
        outFrameExec.dynamicTaskFrameTable = &dynamicTaskFrameTable;

    outExecRuntime.BindViews(
        std::span<ExecNodeRuntime>(_scratch.nodeBacking.get(), graph.nodes.size()),
        std::span<ExecScopeRuntime>(_scratch.scopeBacking.get(), graph.scopeCount));

    return true;
}

void WorldScheduler::FreezeDynamicTaskRequests(
    std::span<WorldRuntime*> runtimeByScope,
    std::span<const WorldId> worldIdByScope,
    uint64_t frameIndex,
    DynamicTaskFrozenBatch& outBatch)
{
    outBatch.Clear();

    for (ExecScopeId scopeId = 0;
        scopeId < static_cast<ExecScopeId>(runtimeByScope.size());
        ++scopeId)
    {
        WorldRuntime* runtime = runtimeByScope[scopeId];
        if (runtime == nullptr)
            continue;

        // pending 요청을 batch에 drain한다.
        // DrainDynamicTaskRequests는 이미 outBatch.requests에 append한다.
        runtime->DrainDynamicTaskRequests(outBatch.requests);
    }

    // Network Inbound 요청 drain
    if (_dynamicTaskScheduler != nullptr)
        _dynamicTaskScheduler->DrainInto(outBatch.requests);

    auto cleanupPayload =
        [this](const DynamicTaskRequest& request)
        {
            if (request.payloadKey == 0 || _dynamicTaskTypeRegistry == nullptr)
                return;

            const DynamicTaskTypeDesc* typeDesc =
                _dynamicTaskTypeRegistry->TryGet(request.typeId);
            if (typeDesc != nullptr && typeDesc->payloadCleanupFn != nullptr)
                typeDesc->payloadCleanupFn(request.payloadKey);
        };

    std::vector<DynamicTaskRequest> resolvedRequests;
    resolvedRequests.reserve(outBatch.requests.size());

    // requestFrameIndex가 설정되지 않은 요청을 현재 프레임으로 보완한다.
    // (Push 시 설정하지 않은 경우를 위한 safety net)
    for (DynamicTaskRequest& req : outBatch.requests)
    {
        if (req.requestFrameIndex == 0)
            req.requestFrameIndex = frameIndex;

        const DynamicTaskTypeDesc* typeDesc =
            _dynamicTaskTypeRegistry != nullptr
                ? _dynamicTaskTypeRegistry->TryGet(req.typeId)
                : nullptr;

        DynamicTaskTargetKind targetKind = req.targetKind;
        if (targetKind == DynamicTaskTargetKind::TypeDefault)
        {
            targetKind = typeDesc != nullptr
                ? typeDesc->defaultTargetKind
                : DynamicTaskTargetKind::ExplicitScope;
        }

        if (targetKind == DynamicTaskTargetKind::SessionCurrentWorld ||
            targetKind == DynamicTaskTargetKind::SessionCurrentWorldOrExplicitScope)
        {
            ExecScopeId resolvedScopeId = InvalidExecScopeId;
            const bool resolved =
                _dynamicTaskScopeResolver != nullptr &&
                _dynamicTaskScopeResolver->TryResolveScope(
                    req,
                    worldIdByScope,
                    resolvedScopeId) &&
                resolvedScopeId != InvalidExecScopeId &&
                resolvedScopeId < static_cast<ExecScopeId>(runtimeByScope.size());

            if (!resolved)
            {
                if (targetKind == DynamicTaskTargetKind::SessionCurrentWorld)
                {
                    cleanupPayload(req);
                    continue;
                }

                if (req.scopeId == InvalidExecScopeId ||
                    req.scopeId >= static_cast<ExecScopeId>(runtimeByScope.size()))
                {
                    cleanupPayload(req);
                    continue;
                }
            }
            else
            {
                req.scopeId = resolvedScopeId;
            }
            req.targetKind = DynamicTaskTargetKind::ExplicitScope;
        }

        resolvedRequests.push_back(std::move(req));
    }

    outBatch.requests = std::move(resolvedRequests);

    // 결정론적 정렬: (scopeId ASC, priorityBias DESC, requestFrameIndex ASC)
    outBatch.Sort();
}
