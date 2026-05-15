#include "pch.h"
#include "AutoSystemBridge.h"

#include <cassert>
#include <string>
#include <unordered_map>

#include "ExecutionContextTypes.h"
#include "System.h"
#include "WorldRuntime.h"
#include "WorldSystemServiceScope.h"

ExecCallResult AutoSystemBridge::BridgeDispatch(NodeExecContext& ctx)
{
    WorldRuntime* worldRuntime = ctx.TryGetRuntime();
    if (worldRuntime == nullptr)
        return ExecCallResult::Failed;

    System* system = worldRuntime->GetSystemByToken(ctx.sourceToken);
    if (system == nullptr)
        return ExecCallResult::Failed;

    WorldSystemServiceScope serviceScope(ctx, *worldRuntime);
    SystemContext sysCtx{
        *worldRuntime,
        worldRuntime->MakeView(),
        worldRuntime->LastDtSec(),
        serviceScope.Services()
    };

    system->Execute(sysCtx);
    return ExecCallResult::Success;
}

ExecNodeKind AutoSystemBridge::PhaseToNodeKind(ExecPhase phase) noexcept
{
    switch (phase) {
    case ExecPhase::Simulate:       return ExecNodeKind::StaticSystem;
    case ExecPhase::Commit:         return ExecNodeKind::PostCommitFinalize;
    case ExecPhase::LifecycleFlush: return ExecNodeKind::LifecycleFlush;
    case ExecPhase::Reconcile:      return ExecNodeKind::Reconcile;
    default:                        return ExecNodeKind::None;
    }
}

const ExecutionSourceDesc* AutoSystemBridge::FindRegisteredSource(
    const ExecutionSourceRegistry& sourceRegistry,
    ExecPhase execPhase,
    std::string_view debugName) noexcept
{
    if (debugName.empty())
        return nullptr;

    for (const ExecutionSourceDesc& source : sourceRegistry.GetSources())
    {
        if (source.phase == execPhase && source.debugName == debugName)
            return &source;
    }

    return nullptr;
}

void AutoSystemBridge::ApplyOrderingHints(
    const std::vector<SystemScheduleDesc>& descs,
    const std::vector<ExecToken>& tokens,
    WorldExecutionModel& outModel)
{
    assert(descs.size() == tokens.size());

    std::unordered_map<ExecTag, ExecToken> tagToToken;
    tagToToken.reserve(descs.size());

    for (size_t i = 0; i < descs.size(); ++i)
    {
        const SystemScheduleDesc& desc = descs[i];
        if (desc.meta == nullptr)
            continue;

        tagToToken.try_emplace(desc.meta->tag, tokens[i]);
    }

    for (size_t i = 0; i < descs.size(); ++i)
    {
        const SystemScheduleDesc& desc = descs[i];
        if (desc.meta == nullptr)
            continue;

        const ExecToken fromToken = tokens[i];

        for (const ExecTag& beforeTag : desc.meta->runsBefore)
        {
            const auto it = tagToToken.find(beforeTag);
            if (it == tagToToken.end())
                continue;

            const ExecToken toToken = it->second;
            if (fromToken == toToken)
                continue;

            outModel.explicitEdges.push_back(
                ExecutionDependencyEdge{ fromToken, toToken });
        }

        for (const ExecTag& afterTag : desc.meta->runsAfter)
        {
            const auto it = tagToToken.find(afterTag);
            if (it == tagToToken.end())
                continue;

            const ExecToken fromTagToken = it->second;
            const ExecToken toToken = tokens[i];
            if (fromTagToken == toToken)
                continue;

            outModel.explicitEdges.push_back(
                ExecutionDependencyEdge{ fromTagToken, toToken });
        }
    }
}

AutoSystemBridge::BridgeResult AutoSystemBridge::RegisterSources(
    ExecPhase execPhase,
    SystemManager& systemManager,
    ExecutionSourceRegistry& sourceRegistry)
{
    BridgeResult result{};

    const ExecNodeKind nodeKind = PhaseToNodeKind(execPhase);
    if (nodeKind == ExecNodeKind::None)
    {
        result.errorMessage =
            "AutoSystemBridge::RegisterSources - unsupported ExecPhase";
        return result;
    }

    const std::vector<SystemScheduleDesc> descs = systemManager.BuildScheduleDescs();

    for (const SystemScheduleDesc& schedDesc : descs)
    {
        if (schedDesc.system == nullptr || schedDesc.meta == nullptr)
        {
            result.errorMessage =
                "AutoSystemBridge::RegisterSources - null system or meta";
            return result;
        }

        // Simulate Phase는 병렬 워커 풀에서 실행되므로 Parallel.
        // 직렬 Phase(Commit/LifecycleFlush/Reconcile)는 메인 스레드가 RunSerialPhase로
        // 직접 순회하므로 Serial. lane 값은 라우팅에 사용되므로 의미론적으로 정확해야 한다.
        const ExecLane lane = (execPhase == ExecPhase::Simulate)
            ? ExecLane::Parallel
            : ExecLane::Serial;

        ExecutionSourceDesc sourceDesc{};
        sourceDesc.token = sourceRegistry.AllocateToken();
        sourceDesc.phase = execPhase;
        sourceDesc.lane = lane;
        sourceDesc.kind = nodeKind;
        sourceDesc.tag = schedDesc.meta->tag;
        sourceDesc.flags = ExecNodeFlag_None;
        sourceDesc.fn = &AutoSystemBridge::BridgeDispatch;
        sourceDesc.debugName = std::string(schedDesc.meta->name);
        sourceDesc.accesses = schedDesc.meta->accesses;
        sourceDesc.runsBefore = schedDesc.meta->runsBefore;
        sourceDesc.runsAfter = schedDesc.meta->runsAfter;
        sourceDesc.schedulingHint = schedDesc.meta->schedulingHint;

        if (!sourceRegistry.Register(sourceDesc))
        {
            result.errorMessage =
                "AutoSystemBridge::RegisterSources - source registration failed";
            return result;
        }

        ++result.registeredCount;
    }

    result.success = true;
    return result;
}

AutoSystemBridge::BridgeResult AutoSystemBridge::BuildModelFromRegisteredSources(
    ExecPhase execPhase,
    SystemManager& systemManager,
    const ExecutionSourceRegistry& sourceRegistry,
    WorldExecutionModel& outModel)
{
    BridgeResult result{};

    std::vector<ExecToken>* phaseTokenList = nullptr;
    switch (execPhase) {
    case ExecPhase::Simulate:
    {
        phaseTokenList = &outModel.simulateSources;
        break;
    }
    case ExecPhase::Commit:
    {
        phaseTokenList = &outModel.commitSources;
        break;
    }
    case ExecPhase::LifecycleFlush:
    {
        phaseTokenList = &outModel.lifecycleFlushSources;
        break;
    }
    case ExecPhase::Reconcile:
        phaseTokenList = &outModel.reconcileSources;
        break;
    default:
    {
        result.errorMessage =
            "AutoSystemBridge::BuildModelFromRegisteredSources - unsupported ExecPhase";
        return result;
    }
    }

    const std::vector<SystemScheduleDesc> descs = systemManager.BuildScheduleDescs();

    std::vector<ExecToken> tokens;
    tokens.reserve(descs.size());

    for (const SystemScheduleDesc& schedDesc : descs)
    {
        if (schedDesc.meta == nullptr)
        {
            result.errorMessage =
                "AutoSystemBridge::BuildModelFromRegisteredSources - null meta";
            return result;
        }

        const ExecutionSourceDesc* source =
            FindRegisteredSource(sourceRegistry, execPhase, schedDesc.meta->name);
        if (source == nullptr)
        {
            result.errorMessage =
                "AutoSystemBridge::BuildModelFromRegisteredSources - missing source";
            return result;
        }

        phaseTokenList->push_back(source->token);
        tokens.push_back(source->token);
        ++result.registeredCount;
    }

    ApplyOrderingHints(descs, tokens, outModel);

    result.success = true;
    return result;
}

AutoSystemBridge::BridgeResult AutoSystemBridge::BindRuntimeDispatch(
    ExecPhase execPhase,
    SystemManager& systemManager,
    const ExecutionSourceRegistry& sourceRegistry,
    WorldRuntime& worldRuntime)
{
    BridgeResult result{};

    const std::vector<SystemScheduleDesc> descs = systemManager.BuildScheduleDescs();

    for (const SystemScheduleDesc& schedDesc : descs)
    {
        if (schedDesc.system == nullptr || schedDesc.meta == nullptr)
        {
            result.errorMessage =
                "AutoSystemBridge::BindRuntimeDispatch - null system or meta";
            return result;
        }

        const ExecutionSourceDesc* source =
            FindRegisteredSource(sourceRegistry, execPhase, schedDesc.meta->name);
        if (source == nullptr)
        {
            result.errorMessage =
                "AutoSystemBridge::BindRuntimeDispatch - missing source";
            return result;
        }

        worldRuntime.RegisterSystemDispatch(source->token, schedDesc.system);
        ++result.registeredCount;
    }

    result.success = true;
    return result;
}

AutoSystemBridge::BridgeResult AutoSystemBridge::Bridge(
    ExecPhase execPhase,
    SystemManager& systemManager,
    ExecutionSourceRegistry& sourceRegistry,
    WorldRuntime& worldRuntime,
    WorldExecutionModel& outModel)
{
    BridgeResult result =
        RegisterSources(execPhase, systemManager, sourceRegistry);
    if (!result.success)
        return result;

    result =
        BuildModelFromRegisteredSources(
            execPhase,
            systemManager,
            sourceRegistry,
            outModel);
    if (!result.success)
        return result;

    return BindRuntimeDispatch(
        execPhase,
        systemManager,
        sourceRegistry,
        worldRuntime);
}
