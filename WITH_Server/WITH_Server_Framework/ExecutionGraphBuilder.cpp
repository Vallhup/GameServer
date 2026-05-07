#include "pch.h"
#include "ExecutionGraphBuilder.h"

#include "FrameworkLog.h"

#include <algorithm>
#include <queue>
#include <string>
#include <utility>
#include <deque>
#include <unordered_map>

#ifdef _DEBUG
#include <fstream>
#include <sstream>
#endif

#include "ExecutionCoreTypes.h"
#include "ExecutionSourceTypes.h"
#include "ConflictDetection.h"

#include "WorldFragmentBuild.h"
#include "WorldExecutionModelTypes.h"

BuildResult ExecutionGraphBuilder::Build(const FrameBuildContext& context)
{
    BuildResult result{};
    result.graph.Clear();
    result.dynamicTaskFrameTable.Clear();
    result.diagnostics.clear();
    result.success = false;

    if (!ValidateBuildContext(context, result))
    {
        result.success = !result.HasError();
        return result;
    }

    std::vector<WorldFragmentBuild> fragments;
    if (!BuildWorldFragments(context, fragments, result))
    {
        result.success = !result.HasError();
        return result;
    }

    const ExecutionGraphBuildPolicy& policy = *context.buildPolicy;

    const bool hasDynamicBatch =
        context.dynamicBatch != nullptr &&
        !context.dynamicBatch->IsEmpty() &&
        context.dynamicTaskTypeRegistry != nullptr;

    // dynamic batch 존재 시 일관성 경고
    if ((context.dynamicBatch != nullptr) != (context.dynamicTaskTypeRegistry != nullptr))
    {
        AddWarning(result,
            "Build - dynamicBatch and dynamicTaskTypeRegistry must both be set or both null");
    }

    if (hasDynamicBatch)
    {
        // [1] 정적 노드 배치 + edge 리스트 구성 (edge pool 미확정)
        std::vector<std::vector<ExecNodeId>> predLists;
        std::vector<std::vector<ExecNodeId>> succLists;

        AssembleFrameGraphNodes(fragments, result.graph, policy, predLists, succLists);

        // [2] 동적 노드 추가 (pred/succ 리스트 확장)
        AppendDynamicNodes(
            *context.dynamicBatch,
            *context.dynamicTaskTypeRegistry,
            *context.executionSourceRegistry,
            context.conflictRegistry,
            result.graph,
            predLists,
            succLists,
            result.dynamicTaskFrameTable,
            result);

        // [3] edge pool 직렬화
        FinalizeEdgePool(result.graph, predLists, succLists, policy);
    }
    else
    {
        // static-only fast path (기존 경로 유지)
        AssembleFrameGraph(fragments, result.graph, policy);
    }

    if (policy.applyTransitiveReduction)
        ApplyTransitiveReduction(result.graph);

    BuildSerialExecutionPlan(result.graph, policy);

    if (!ValidateGraph(result.graph, policy, result))
    {
        result.success = !result.HasError();
        return result;
    }

    result.success = !result.HasError();

#if defined(_DEBUG)
    if (result.success && context.executionSourceRegistry != nullptr)
        WriteDebugGraph(result.graph, *context.executionSourceRegistry);
#endif

    return result;
}

bool ExecutionGraphBuilder::ValidateBuildContext(
    const FrameBuildContext& context,
    BuildResult& result) const
{
    if (!context.IsValid())
    {
        AddError(result, "ValidateBuildContext - FrameBuildContext is invalid");
        return false;
    }

    if (context.frameSelectionSet == nullptr)
    {
        AddError(result, "ValidateBuildContext - frameSelectionSet is null");
        return false;
    }

    if (context.executionModelRegistry == nullptr)
    {
        AddError(result, "ValidateBuildContext - executionModelRegistry is null");
        return false;
    }

    if (context.executionSourceRegistry == nullptr)
    {
        AddError(result, "ValidateBuildContext - executionSourceRegistry is null");
        return false;
    }

    if (context.buildPolicy == nullptr)
    {
        AddError(result, "ValidateBuildContext - buildPolicy is null");
        return false;
    }

    const ExecutionGraphBuildPolicy& policy = *context.buildPolicy;

    if (context.frameSelectionSet->IsEmpty())
    {
        ReportByPolicy(
            result,
            policy.emptyFrameSelection,
            "ValidateBuildContext - frameSelectionSet is empty");

        if (policy.emptyFrameSelection == BuildDecision::Error)
            return false;
    }

    return true;
}

bool ExecutionGraphBuilder::BuildWorldFragments(
    const FrameBuildContext& context,
    std::vector<WorldFragmentBuild>& outFragments,
    BuildResult& result) const
{
    outFragments.clear();

    if (!context.IsValid())
    {
        AddError(result, "BuildWorldFragments - invalid build context");
        return false;
    }

    const WorldFrameSelectionSet& frameSet = *context.frameSelectionSet;
    const WorldExecutionModelRegistry& modelRegistry = *context.executionModelRegistry;
    const ExecutionSourceRegistry& sourceRegistry = *context.executionSourceRegistry;
    const ExecutionGraphBuildPolicy& policy = *context.buildPolicy;

    if (frameSet.IsEmpty())
    {
        ReportByPolicy(
            result,
            policy.emptyFrameSelection,
            "BuildWorldFragments - frame selection set is empty");

        return policy.emptyFrameSelection != BuildDecision::Error;
    }

    outFragments.reserve(frameSet.GetCount());

    for (const WorldFrameSelection& selection : frameSet.selections)
    {
        if (!selection.IsValid())
        {
            ReportByPolicy(
                result,
                policy.invalidSelection,
                "BuildWorldFragments - invalid world frame selection");
            return false;
        }

        const WorldExecutionModel* model = modelRegistry.TryGet(selection.modelKey);
        if (model == nullptr)
        {
            ReportByPolicy(
                result,
                policy.missingExecutionModel,
                "BuildWorldFragments - execution model not found");
            return false;
        }

        if (!model->IsValid())
        {
            ReportByPolicy(
                result,
                policy.invalidExecutionModel,
                "BuildWorldFragments - invalid execution model");
            return false;
        }

        WorldFragmentBuild fragment{};
        if (!BuildSingleWorldFragment(
                selection, *model, sourceRegistry, policy,
                context.conflictRegistry, fragment, result))
            return false;

        outFragments.push_back(std::move(fragment));
    }

    return true;
}
bool ExecutionGraphBuilder::BuildSingleWorldFragment(
    const WorldFrameSelection& selection,
    const WorldExecutionModel& model,
    const ExecutionSourceRegistry& sourceRegistry,
    const ExecutionGraphBuildPolicy& policy,
    const ConflictRegistry* conflictRegistry,
    WorldFragmentBuild& outFragment,
    BuildResult& result) const
{
    outFragment.Clear();
    outFragment.scopeId = selection.scopeId;
    outFragment.worldBinding = selection.worldBinding;
    outFragment.modelKey = selection.modelKey;

    struct TokenPlacement
    {
        ExecToken token{ InvalidExecToken };
        ExecPhase phase{ ExecPhase::None };
    };

    std::vector<TokenPlacement> orderedTokens;
    orderedTokens.reserve(model.GetSourceCount());

    auto appendTokens =
        [&](const std::vector<ExecToken>& tokens, ExecPhase phase)
        {
            for (ExecToken token : tokens)
                orderedTokens.push_back({ token, phase });
        };

    appendTokens(model.simulateSources, ExecPhase::Simulate);
    appendTokens(model.commitSources, ExecPhase::Commit);
    appendTokens(model.lifecycleFlushSources, ExecPhase::LifecycleFlush);
    appendTokens(model.reconcileSources, ExecPhase::Reconcile);

    std::unordered_map<ExecToken, uint32_t> tokenToLocalIndex;
    tokenToLocalIndex.reserve(orderedTokens.size());

    for (const TokenPlacement& placement : orderedTokens)
    {
        if (placement.token == InvalidExecToken)
        {
            ReportByPolicy(
                result,
                policy.invalidExecutionSource,
                "BuildSingleWorldFragment - invalid source token");
            return false;
        }

        if (tokenToLocalIndex.find(placement.token) != tokenToLocalIndex.end())
        {
            ReportByPolicy(
                result,
                policy.duplicateSourceTokenInModel,
                "BuildSingleWorldFragment - duplicated source token in model");
            return false;
        }

        const ExecutionSourceDesc* desc = sourceRegistry.TryGet(placement.token);
        if (desc == nullptr)
        {
            ReportByPolicy(
                result,
                policy.missingExecutionSource,
                "BuildSingleWorldFragment - source descriptor not found");
            return false;
        }

        if (!desc->IsValid())
        {
            ReportByPolicy(
                result,
                policy.invalidExecutionSource,
                "BuildSingleWorldFragment - invalid source descriptor");
            return false;
        }

        if (desc->phase != placement.phase)
        {
            ReportByPolicy(
                result,
                policy.sourcePhaseMismatch,
                "BuildSingleWorldFragment - source phase mismatch");
            return false;
        }

        ExecNodeRecord node{};
        node.id = InvalidExecNodeId;
        node.scopeId = selection.scopeId;
        node.phase = desc->phase;
        node.lane = desc->lane;
        node.kind = desc->kind;
        node.flags = desc->flags;
        node.predBegin = 0;
        node.predCount = 0;
        node.succBegin = 0;
        node.succCount = 0;
        node.debugNameOffset = 0;
        node.sourceToken = desc->token;
        node.priorityBias = desc->schedulingHint.priorityBias;

        const uint32_t localIndex =
            static_cast<uint32_t>(outFragment.localNodes.size());

        outFragment.localNodes.push_back(node);
        tokenToLocalIndex.try_emplace(desc->token, localIndex);
    }

    if (!BuildFragmentEdges(
            model, sourceRegistry, tokenToLocalIndex,
            policy, conflictRegistry, outFragment, result))
        return false;

    if (!ValidateFragmentAcyclicPerPhase(outFragment, policy, result))
        return false;

    return true;
}

bool ExecutionGraphBuilder::BuildFragmentEdges(
    const WorldExecutionModel& model,
    const ExecutionSourceRegistry& sourceRegistry,
    const std::unordered_map<ExecToken, uint32_t>& tokenToLocalIndex,
    const ExecutionGraphBuildPolicy& policy,
    const ConflictRegistry* conflictRegistry,
    WorldFragmentBuild& outFragment,
    BuildResult& result) const
{
    auto phaseRank =
        [](ExecPhase phase) -> int
        {
            switch (phase) {
            case ExecPhase::Simulate:       return 0;
            case ExecPhase::Commit:         return 1;
            case ExecPhase::LifecycleFlush: return 2;
            case ExecPhase::Reconcile:      return 3;
            default:                        return -1;
            }
        };

    for (const ExecutionDependencyEdge& edge : model.explicitEdges)
    {
        if (!edge.IsValid())
        {
            ReportByPolicy(
                result,
                policy.invalidDependencyEdge,
                "BuildFragmentEdges - invalid dependency edge");
            return false;
        }

        if (edge.fromToken == edge.toToken)
        {
            ReportByPolicy(
                result,
                policy.selfDependencyEdge,
                "BuildFragmentEdges - self-edge is not allowed");
            return false;
        }

        const ExecPhase fromPhase = model.TryGetDeclaredPhase(edge.fromToken);
        const ExecPhase toPhase = model.TryGetDeclaredPhase(edge.toToken);

        if (fromPhase == ExecPhase::None || toPhase == ExecPhase::None)
        {
            ReportByPolicy(
                result,
                policy.edgeOutsideModel,
                "BuildFragmentEdges - dependency edge references token outside model");
            return false;
        }

        if (phaseRank(fromPhase) > phaseRank(toPhase))
        {
            ReportByPolicy(
                result,
                policy.backwardPhaseDependency,
                "BuildFragmentEdges - backward phase dependency is not allowed");
            return false;
        }

        if (!policy.allowCrossPhaseExplicitEdges && fromPhase != toPhase)
        {
            ReportByPolicy(
                result,
                policy.crossPhaseExplicitEdgeDisallowed,
                "BuildFragmentEdges - cross-phase dependency is disallowed by policy");
            return false;
        }

        const auto fromIt = tokenToLocalIndex.find(edge.fromToken);
        const auto toIt = tokenToLocalIndex.find(edge.toToken);

        if (fromIt == tokenToLocalIndex.end() || toIt == tokenToLocalIndex.end())
        {
            ReportByPolicy(
                result,
                policy.edgeOutsideModel,
                "BuildFragmentEdges - failed to remap token to local index");
            return false;
        }

        LocalFragmentEdge localEdge{};
        localEdge.fromLocalIndex = fromIt->second;
        localEdge.toLocalIndex = toIt->second;
        outFragment.localEdges.push_back(localEdge);
    }

    // -----------------------------------------------------------------------
    // Accesses 기반 자동 의존성 엣지 추론
    //
    // 동일 Phase의 노드 쌍을 순회하며 AccessSpec 충돌이 감지되면 엣지를 삽입한다.
    // 결정성을 위해 localIndex 오름차순(i < j) 방향으로만 엣지를 추가한다.
    // 이미 존재하는 explicit edge와 중복되는 경우 삽입하지 않는다.
    // -----------------------------------------------------------------------
    const uint32_t nodeCount = static_cast<uint32_t>(outFragment.localNodes.size());

    // 기존 엣지 집합을 빠른 중복 검사를 위해 인덱싱한다.
    // key: (fromLocalIndex << 32) | toLocalIndex — uint64_t 상위/하위 32비트에 각각 배치.
    // 노드 인덱스는 uint32_t이므로 최대 ~43억 노드까지 충돌 없이 안전하다.
    auto makeEdgeKey = [](uint32_t from, uint32_t to) -> uint64_t
    {
        return (static_cast<uint64_t>(from) << 32) | static_cast<uint64_t>(to);
    };

    std::unordered_map<uint64_t, bool> existingEdgeSet;
    existingEdgeSet.reserve(outFragment.localEdges.size() * 2);

    for (const LocalFragmentEdge& e : outFragment.localEdges)
    {
        existingEdgeSet.emplace(makeEdgeKey(e.fromLocalIndex, e.toLocalIndex), true);
        existingEdgeSet.emplace(makeEdgeKey(e.toLocalIndex, e.fromLocalIndex), true);
    }

    for (uint32_t i = 0; i < nodeCount; ++i)
    {
        const ExecNodeRecord& nodeI = outFragment.localNodes[i];
        const ExecutionSourceDesc* descI = sourceRegistry.TryGet(nodeI.sourceToken);

        if (descI == nullptr || descI->accesses.empty())
            continue;

        for (uint32_t j = i + 1; j < nodeCount; ++j)
        {
            const ExecNodeRecord& nodeJ = outFragment.localNodes[j];

            // 서로 다른 Phase 간 자동 엣지는 생성하지 않는다.
            // 크로스 Phase 의존성은 Phase 순서 자체가 보장한다.
            if (nodeI.phase != nodeJ.phase)
                continue;

            const ExecutionSourceDesc* descJ = sourceRegistry.TryGet(nodeJ.sourceToken);

            if (descJ == nullptr || descJ->accesses.empty())
                continue;

            if (!HasAnyConflict(descI->accesses, descJ->accesses, conflictRegistry))
                continue;

            // i → j 방향 엣지가 아직 없으면 삽입한다.
            const uint64_t keyIJ = makeEdgeKey(i, j);
            if (existingEdgeSet.find(keyIJ) == existingEdgeSet.end())
            {
                LocalFragmentEdge autoEdge{};
                autoEdge.fromLocalIndex = i;
                autoEdge.toLocalIndex = j;
                outFragment.localEdges.push_back(autoEdge);

                existingEdgeSet.emplace(keyIJ, true);
                existingEdgeSet.emplace(makeEdgeKey(j, i), true);
            }
        }
    }

    return true;
}

bool ExecutionGraphBuilder::ValidateFragmentAcyclicPerPhase(
    const WorldFragmentBuild& fragment,
    const ExecutionGraphBuildPolicy& policy,
    BuildResult& result) const
{
    if (!fragment.IsValid())
    {
        AddError(result, "ValidateFragmentAcyclicPerPhase - invalid fragment");
        return false;
    }

    auto validatePhase = 
        [&](ExecPhase phase, const char* phaseName) -> bool
        {
            // 1) 이 phase에 속한 local node만 수집
            std::vector<uint32_t> phaseLocalIndices;
            phaseLocalIndices.reserve(fragment.localNodes.size());

            for (uint32_t localIndex = 0;
                localIndex < static_cast<uint32_t>(fragment.localNodes.size());
                ++localIndex)
            {
                if (fragment.localNodes[localIndex].phase == phase)
                    phaseLocalIndices.push_back(localIndex);
            }

            // node가 0개나 1개면 cycle 없음
            if (phaseLocalIndices.size() <= 1)
                return true;

            // 2) localIndex -> compactIndex 매핑
            std::unordered_map<uint32_t, uint32_t> localToCompact;
            localToCompact.reserve(phaseLocalIndices.size());

            for (uint32_t compactIndex = 0;
                compactIndex < static_cast<uint32_t>(phaseLocalIndices.size());
                ++compactIndex)
            {
                localToCompact.emplace(phaseLocalIndices[compactIndex], compactIndex);
            }

            // 3) same-phase edge만 뽑아서 indegree/adjacency 구성
            std::vector<uint32_t> indegree(phaseLocalIndices.size(), 0);
            std::vector<std::vector<uint32_t>> adjacency(phaseLocalIndices.size());

            for (const LocalFragmentEdge& edge : fragment.localEdges)
            {
                if (!fragment.IsValidLocalIndex(edge.fromLocalIndex) ||
                    !fragment.IsValidLocalIndex(edge.toLocalIndex))
                {
                    AddError(result, "ValidateFragmentAcyclicPerPhase - edge has invalid local index");
                    return false;
                }

                const ExecNodeRecord& fromNode = fragment.localNodes[edge.fromLocalIndex];
                const ExecNodeRecord& toNode = fragment.localNodes[edge.toLocalIndex];

                // cross-phase edge는 cycle 검사 대상 아님
                if (fromNode.phase != phase || toNode.phase != phase)
                    continue;

                const auto fromIt = localToCompact.find(edge.fromLocalIndex);
                const auto toIt = localToCompact.find(edge.toLocalIndex);

                if (fromIt == localToCompact.end() || toIt == localToCompact.end())
                {
                    AddError(result, "ValidateFragmentAcyclicPerPhase - failed to remap local index");
                    return false;
                }

                const uint32_t fromCompact = fromIt->second;
                const uint32_t toCompact = toIt->second;

                adjacency[fromCompact].push_back(toCompact);
                ++indegree[toCompact];
            }

            // 4) stable topo sort
            // local index 오름차순 안정성을 유지하려면
            // compactIndex 자체가 phaseLocalIndices 입력 순서를 따르게 둔다.
            std::deque<uint32_t> ready;

            for (uint32_t i = 0; i < static_cast<uint32_t>(indegree.size()); ++i)
            {
                if (indegree[i] == 0)
                    ready.push_back(i);
            }

            uint32_t visitedCount = 0;

            while (!ready.empty())
            {
                const uint32_t cur = ready.front();
                ready.pop_front();
                ++visitedCount;

                for (uint32_t next : adjacency[cur])
                {
                    if (indegree[next] == 0)
                    {
                        AddError(result, "ValidateFragmentAcyclicPerPhase - indegree underflow");
                        return false;
                    }

                    --indegree[next];
                    if (indegree[next] == 0)
                        ready.push_back(next);
                }
            }

            if (visitedCount != static_cast<uint32_t>(phaseLocalIndices.size()))
            {
                std::string msg =
                    "ValidateFragmentAcyclicPerPhase - cycle detected in phase: ";
                msg += phaseName;

                ReportByPolicy(result, policy.samePhaseCycle, msg.c_str());
                return false;
            }

            return true;
        };

    if (!validatePhase(ExecPhase::Simulate, "Simulate"))
        return false;

    if (!validatePhase(ExecPhase::Commit, "Commit"))
        return false;

    if (!validatePhase(ExecPhase::LifecycleFlush, "LifecycleFlush"))
        return false;

    if (!validatePhase(ExecPhase::Reconcile, "Reconcile"))
        return false;

    return true;
}

// ---------------------------------------------------------------------------
// AssembleFrameGraphNodes [1/3]
// ---------------------------------------------------------------------------
void ExecutionGraphBuilder::AssembleFrameGraphNodes(
    const std::vector<WorldFragmentBuild>& fragments,
    FrameTaskGraph& outGraph,
    const ExecutionGraphBuildPolicy& policy,
    std::vector<std::vector<ExecNodeId>>& outPredLists,
    std::vector<std::vector<ExecNodeId>>& outSuccLists) const
{
    outGraph.Clear();
    outPredLists.clear();
    outSuccLists.clear();

    if (fragments.empty())
        return;

    ExecScopeId maxScopeId = InvalidExecScopeId;
    bool hasValidScope = false;

    for (const WorldFragmentBuild& fragment : fragments)
    {
        if (!fragment.IsValid())
            continue;

        if (!hasValidScope || fragment.scopeId > maxScopeId)
        {
            maxScopeId = fragment.scopeId;
            hasValidScope = true;
        }
    }

    if (hasValidScope)
    {
        outGraph.scopeCount = static_cast<uint32_t>(maxScopeId + 1);
        outGraph.scopeToWorld.resize(outGraph.scopeCount, InvalidWorldBinding);
    }

    uint32_t totalNodeCount = 0;
    for (const WorldFragmentBuild& fragment : fragments)
        totalNodeCount += static_cast<uint32_t>(fragment.localNodes.size());

    outGraph.nodes.reserve(totalNodeCount);
    outPredLists.resize(totalNodeCount);
    outSuccLists.resize(totalNodeCount);

    struct FragmentRemap
    {
        const WorldFragmentBuild* fragment{ nullptr };
        std::vector<ExecNodeId>   localToGlobal;
    };

    std::vector<FragmentRemap> remaps;
    remaps.reserve(fragments.size());

    ExecNodeId nextGlobalNodeId = 0;

    for (const WorldFragmentBuild& fragment : fragments)
    {
        FragmentRemap remap{};
        remap.fragment = &fragment;
        remap.localToGlobal.resize(fragment.localNodes.size(), InvalidExecNodeId);

        if (fragment.scopeId != InvalidExecScopeId &&
            fragment.scopeId < outGraph.scopeToWorld.size())
        {
            outGraph.scopeToWorld[fragment.scopeId] = fragment.worldBinding;
        }

        for (uint32_t localIndex = 0;
            localIndex < static_cast<uint32_t>(fragment.localNodes.size());
            ++localIndex)
        {
            ExecNodeRecord node = fragment.localNodes[localIndex];
            node.id = nextGlobalNodeId;
            node.predBegin = 0;
            node.predCount = 0;
            node.succBegin = 0;
            node.succCount = 0;

            remap.localToGlobal[localIndex] = nextGlobalNodeId;
            outGraph.nodes.push_back(node);

            if (node.phase == ExecPhase::Simulate)
                ++outGraph.simulateNodeCount;

            ++nextGlobalNodeId;
        }

        remaps.push_back(std::move(remap));
    }

    auto appendUnique = [&](std::vector<ExecNodeId>& list, ExecNodeId value)
    {
        if (!policy.deduplicateSamePhaseEdges)
        {
            list.push_back(value);
            return;
        }
        for (ExecNodeId existing : list)
        {
            if (existing == value)
                return;
        }
        list.push_back(value);
    };

    for (const FragmentRemap& remap : remaps)
    {
        const WorldFragmentBuild& fragment = *remap.fragment;

        for (const LocalFragmentEdge& edge : fragment.localEdges)
        {
            if (!fragment.IsValidLocalIndex(edge.fromLocalIndex) ||
                !fragment.IsValidLocalIndex(edge.toLocalIndex))
            {
                continue;
            }

            const ExecNodeId fromGlobal = remap.localToGlobal[edge.fromLocalIndex];
            const ExecNodeId toGlobal   = remap.localToGlobal[edge.toLocalIndex];

            if (fromGlobal == InvalidExecNodeId || toGlobal == InvalidExecNodeId)
                continue;

            const ExecPhase fromPhase = outGraph.nodes[fromGlobal].phase;
            const ExecPhase toPhase   = outGraph.nodes[toGlobal].phase;

            if (fromPhase != toPhase && !policy.materializeCrossPhaseRuntimeDeps)
                continue;

            appendUnique(outSuccLists[fromGlobal], toGlobal);
            appendUnique(outPredLists[toGlobal], fromGlobal);
        }
    }
}

// ---------------------------------------------------------------------------
// AppendDynamicNodes [2/3]
// ---------------------------------------------------------------------------
void ExecutionGraphBuilder::AppendDynamicNodes(
    const DynamicTaskFrozenBatch& batch,
    const DynamicTaskTypeRegistry& typeRegistry,
    const ExecutionSourceRegistry& sourceRegistry,
    const ConflictRegistry* conflictRegistry,
    FrameTaskGraph& outGraph,
    std::vector<std::vector<ExecNodeId>>& predLists,
    std::vector<std::vector<ExecNodeId>>& succLists,
    DynamicTaskFrameTable& outFrameTable,
    BuildResult& result) const
{
    if (batch.IsEmpty())
        return;

    std::unordered_map<uint64_t, ExecNodeId> lastDynamicNodeBySessionAndPhase;
    const auto makeSessionPhaseKey =
        [](uint32_t sessionId, ExecPhase phase) noexcept -> uint64_t
        {
            return (static_cast<uint64_t>(sessionId) << 32)
                | static_cast<uint32_t>(phase);
        };
    const auto appendUniqueEdge =
        [](std::vector<ExecNodeId>& preds,
           std::vector<ExecNodeId>& succs,
           ExecNodeId pred,
           ExecNodeId succ)
        {
            if (std::find(preds.begin(), preds.end(), pred) == preds.end())
            {
                preds.push_back(pred);
                succs.push_back(succ);
            }
        };

    for (const DynamicTaskRequest& request : batch.requests)
    {
        if (!request.IsValid())
        {
            AddWarning(result, "AppendDynamicNodes - invalid request (skipped)");
            continue;
        }

        const DynamicTaskTypeDesc* typeDesc = typeRegistry.TryGet(request.typeId);
        if (typeDesc == nullptr)
        {
            AddWarning(result, "AppendDynamicNodes - DynamicTaskTypeDesc not found (skipped)");
            continue;
        }

        const ExecutionSourceDesc* sourceDesc = sourceRegistry.TryGet(typeDesc->sourceToken);
        if (sourceDesc == nullptr)
        {
            AddError(result, "AppendDynamicNodes - ExecutionSourceDesc not found");
            continue;
        }

        if (outGraph.scopeCount > 0 && request.scopeId >= outGraph.scopeCount)
        {
            const std::string message =
                "AppendDynamicNodes - scopeId out of range (skipped: typeId=" +
                std::to_string(request.typeId) +
                ", scopeId=" +
                std::to_string(request.scopeId) +
                ", sessionId=" +
                std::to_string(request.sessionId) +
                ", scopeCount=" +
                std::to_string(outGraph.scopeCount) +
                ")";
            AddWarning(result, message.c_str());
            continue;
        }

        const ExecNodeId newNodeId = static_cast<ExecNodeId>(outGraph.nodes.size());

        ExecNodeRecord record{};
        record.id           = newNodeId;
        record.scopeId      = request.scopeId;
        record.phase        = typeDesc->defaultPhase;
        record.lane         = typeDesc->defaultLane;
        record.kind         = ExecNodeKind::DynamicTask;
        record.flags        = typeDesc->flags;
        record.sourceToken  = typeDesc->sourceToken;
        record.priorityBias = request.priorityBias + typeDesc->schedulingHint.priorityBias;

        record.debugNameOffset = static_cast<uint32_t>(outGraph.debugNameBlob.size());
        outGraph.debugNameBlob += typeDesc->debugName;
        outGraph.debugNameBlob += '\0';

        // pred/succ 리스트 공간 확보
        predLists.emplace_back();
        succLists.emplace_back();

        // access 충돌 감지: 같은 scope + phase의 기존 노드 전체와 비교
        const std::span<const AccessSpec> newAccesses{
            typeDesc->accesses.data(), typeDesc->accesses.size() };

        if (request.sessionId != 0)
        {
            const uint64_t sessionPhaseKey =
                makeSessionPhaseKey(request.sessionId, typeDesc->defaultPhase);
            const auto prevIt =
                lastDynamicNodeBySessionAndPhase.find(sessionPhaseKey);
            if (prevIt != lastDynamicNodeBySessionAndPhase.end())
            {
                appendUniqueEdge(
                    predLists[newNodeId],
                    succLists[prevIt->second],
                    prevIt->second,
                    newNodeId);
            }
        }

        for (ExecNodeId existingId = 0; existingId < newNodeId; ++existingId)
        {
            const ExecNodeRecord& existing = outGraph.nodes[existingId];

            if (existing.scopeId != request.scopeId)
                continue;
            if (existing.phase != typeDesc->defaultPhase)
                continue;

            const ExecutionSourceDesc* existingSource =
                sourceRegistry.TryGet(existing.sourceToken);
            if (existingSource == nullptr || existingSource->accesses.empty())
                continue;
            if (newAccesses.empty())
                continue;

            if (HasAnyConflict(newAccesses, existingSource->accesses, conflictRegistry))
            {
                appendUniqueEdge(
                    predLists[newNodeId],
                    succLists[existingId],
                    existingId,
                    newNodeId);
            }
        }

        outGraph.nodes.push_back(record);

        if (request.sessionId != 0)
        {
            const uint64_t sessionPhaseKey =
                makeSessionPhaseKey(request.sessionId, typeDesc->defaultPhase);
            lastDynamicNodeBySessionAndPhase[sessionPhaseKey] = newNodeId;
        }

        if (record.phase == ExecPhase::Simulate)
            ++outGraph.simulateNodeCount;

        DynamicTaskInstance instance{};
        instance.typeId      = request.typeId;
        instance.scopeId     = request.scopeId;
        instance.sessionId   = request.sessionId;
        instance.graphNodeId = newNodeId;
        instance.payloadKey  = request.payloadKey;
        outFrameTable.AddInstance(instance);
    }
}

// ---------------------------------------------------------------------------
// FinalizeEdgePool [3/3]
// ---------------------------------------------------------------------------
void ExecutionGraphBuilder::FinalizeEdgePool(
    FrameTaskGraph& outGraph,
    const std::vector<std::vector<ExecNodeId>>& predLists,
    const std::vector<std::vector<ExecNodeId>>& succLists,
    const ExecutionGraphBuildPolicy& /*policy*/) const
{
    const uint32_t nodeCount = static_cast<uint32_t>(outGraph.nodes.size());
    if (nodeCount == 0)
        return;

    size_t totalEdges = 0;
    for (uint32_t i = 0; i < nodeCount; ++i)
    {
        if (i < predLists.size()) totalEdges += predLists[i].size();
        if (i < succLists.size()) totalEdges += succLists[i].size();
    }
    outGraph.edges.reserve(totalEdges);

    for (ExecNodeId nodeId = 0; nodeId < nodeCount; ++nodeId)
    {
        ExecNodeRecord& node = outGraph.nodes[nodeId];

        node.predBegin = static_cast<uint32_t>(outGraph.edges.size());
        if (nodeId < static_cast<ExecNodeId>(predLists.size()))
        {
            node.predCount = static_cast<uint32_t>(predLists[nodeId].size());
            for (ExecNodeId pred : predLists[nodeId])
                outGraph.edges.push_back(pred);
        }
        else
        {
            node.predCount = 0;
        }

        node.succBegin = static_cast<uint32_t>(outGraph.edges.size());
        if (nodeId < static_cast<ExecNodeId>(succLists.size()))
        {
            node.succCount = static_cast<uint32_t>(succLists[nodeId].size());
            for (ExecNodeId succ : succLists[nodeId])
                outGraph.edges.push_back(succ);
        }
        else
        {
            node.succCount = 0;
        }
    }
}

// ---------------------------------------------------------------------------
// AssembleFrameGraph — 레거시 래퍼 (dynamic batch 없는 경로)
// ---------------------------------------------------------------------------
void ExecutionGraphBuilder::AssembleFrameGraph(
    const std::vector<WorldFragmentBuild>& fragments,
    FrameTaskGraph& outGraph,
    const ExecutionGraphBuildPolicy& policy) const
{
    std::vector<std::vector<ExecNodeId>> predLists;
    std::vector<std::vector<ExecNodeId>> succLists;
    AssembleFrameGraphNodes(fragments, outGraph, policy, predLists, succLists);
    FinalizeEdgePool(outGraph, predLists, succLists, policy);
}

void ExecutionGraphBuilder::BuildSerialExecutionPlan(
    FrameTaskGraph& graph,
    const ExecutionGraphBuildPolicy& policy) const
{
    (void)policy;

    graph.serialExecutionOrder.clear();
    graph.serialScopeOrder.clear();
    graph.commitPlan = {};
    graph.lifecycleFlushPlan = {};
    graph.reconcilePlan = {};
    graph.commitScopePlan = {};
    graph.lifecycleFlushScopePlan = {};
    graph.reconcileScopePlan = {};

    auto buildPhasePlan = 
        [&](ExecPhase phase, ExecRange& outRange)
        {
            // 1) phase node 수집
            std::vector<ExecNodeId> phaseNodes;
            phaseNodes.reserve(graph.nodes.size());

            for (ExecNodeId nodeId = 0;
                nodeId < static_cast<ExecNodeId>(graph.nodes.size());
                ++nodeId)
            {
                const ExecNodeRecord& node = graph.nodes[nodeId];
                if (node.phase == phase)
                    phaseNodes.push_back(nodeId);
            }

            outRange.begin = static_cast<uint32_t>(graph.serialExecutionOrder.size());
            outRange.count = 0;

            if (phaseNodes.empty())
                return;

            // ExecNodeId 오름차순 stable order를 위해 정렬
            std::sort(phaseNodes.begin(), phaseNodes.end());

            // 2) global node id -> compact index
            std::vector<int32_t> nodeToCompact(graph.nodes.size(), -1);
            for (uint32_t i = 0; i < static_cast<uint32_t>(phaseNodes.size()); ++i)
                nodeToCompact[phaseNodes[i]] = static_cast<int32_t>(i);

            // 3) phase 내부 edge 추출 + indegree 계산
            std::vector<uint32_t> indegree(phaseNodes.size(), 0);
            std::vector<std::vector<uint32_t>> adjacency(phaseNodes.size());

            for (uint32_t compactFrom = 0;
                compactFrom < static_cast<uint32_t>(phaseNodes.size());
                ++compactFrom)
            {
                const ExecNodeId fromNodeId = phaseNodes[compactFrom];
                const ExecNodeRecord& fromNode = graph.nodes[fromNodeId];

                // same-phase succ만 사용
                for (uint32_t i = 0; i < fromNode.succCount; ++i)
                {
                    const uint32_t edgeIndex = fromNode.succBegin + i;
                    if (edgeIndex >= graph.edges.size())
                        continue;

                    const ExecNodeId toNodeId = graph.edges[edgeIndex];
                    if (!graph.IsValidNodeId(toNodeId))
                        continue;

                    const ExecNodeRecord& toNode = graph.nodes[toNodeId];
                    if (toNode.phase != phase)
                        continue;

                    const int32_t compactTo = nodeToCompact[toNodeId];
                    if (compactTo < 0)
                        continue;

                    adjacency[compactFrom].push_back(static_cast<uint32_t>(compactTo));
                    ++indegree[static_cast<uint32_t>(compactTo)];
                }
            }

            // 4) stable topo sort (ExecNodeId 오름차순)
            using ReadyItem = std::pair<ExecNodeId, uint32_t>; // {nodeId, compactIndex}
            auto cmp = 
                [](const ReadyItem& a, const ReadyItem& b)
                {
                    return a.first > b.first; // min-heap by ExecNodeId
                };

            std::priority_queue<ReadyItem, std::vector<ReadyItem>, decltype(cmp)> ready(cmp);

            for (uint32_t i = 0; i < static_cast<uint32_t>(phaseNodes.size()); ++i)
            {
                if (indegree[i] == 0)
                    ready.push({ phaseNodes[i], i });
            }

            uint32_t appendedCount = 0;

            while (!ready.empty())
            {
                const auto [nodeId, compactIndex] = ready.top();
                ready.pop();

                graph.serialExecutionOrder.push_back(nodeId);
                ++appendedCount;

                for (uint32_t nextCompact : adjacency[compactIndex])
                {
                    if (indegree[nextCompact] == 0)
                        continue; // cycle 검사는 ValidateGraph/이전 단계에서 처리

                    --indegree[nextCompact];
                    if (indegree[nextCompact] == 0)
                    {
                        ready.push({ phaseNodes[nextCompact], nextCompact });
                    }
                }
            }

            outRange.count = appendedCount;
        };

    auto buildScopePlan =
        [&](ExecRange& outRange)
        {
            outRange.begin = static_cast<uint32_t>(graph.serialScopeOrder.size());
            outRange.count = 0;

            if (graph.scopeToWorld.empty())
                return;

            for (ExecScopeId scopeId = 0;
                scopeId < static_cast<ExecScopeId>(graph.scopeToWorld.size());
                ++scopeId)
            {
                if (graph.scopeToWorld[scopeId] == InvalidWorldBinding)
                    continue;

                graph.serialScopeOrder.push_back(scopeId);
                ++outRange.count;
            }
        };

    buildPhasePlan(ExecPhase::Commit, graph.commitPlan);
    buildPhasePlan(ExecPhase::LifecycleFlush, graph.lifecycleFlushPlan);
    buildPhasePlan(ExecPhase::Reconcile, graph.reconcilePlan);

    buildScopePlan(graph.commitScopePlan);
    buildScopePlan(graph.lifecycleFlushScopePlan);
    buildScopePlan(graph.reconcileScopePlan);
}

bool ExecutionGraphBuilder::ValidateGraph(
    const FrameTaskGraph& graph,
    const ExecutionGraphBuildPolicy& policy,
    BuildResult& result
) const
{
    bool ok = true;

    auto fail = [&](const char* msg)
        {
            AddError(result, msg);
            ok = false;
        };

    // 1) scope table 정합성
    if (graph.scopeCount != static_cast<uint32_t>(graph.scopeToWorld.size()))
    {
        fail("ValidateGraph - scopeCount does not match scopeToWorld size");
    }

    if (!graph.IsValidSerialScopeRange(graph.commitScopePlan))
    {
        fail("ValidateGraph - commitScopePlan range out of bounds");
    }

    if (!graph.IsValidSerialScopeRange(graph.lifecycleFlushScopePlan))
    {
        fail("ValidateGraph - lifecycleFlushScopePlan range out of bounds");
    }

    if (!graph.IsValidSerialScopeRange(graph.reconcileScopePlan))
    {
        fail("ValidateGraph - reconcileScopePlan range out of bounds");
    }

    if (policy.requireDenseScopeIds)
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(graph.scopeToWorld.size()); ++i)
        {
            if (graph.scopeToWorld[i] == InvalidWorldBinding)
            {
                fail("ValidateGraph - dense scope policy violated: empty scopeToWorld slot");
            }
        }
    }

    // 2) node 기본 정합성 + pred/succ range 검사
    uint32_t simulateCount = 0;

    for (uint32_t expectedNodeId = 0;
        expectedNodeId < static_cast<uint32_t>(graph.nodes.size());
        ++expectedNodeId)
    {
        const ExecNodeRecord& node = graph.nodes[expectedNodeId];

        if (node.id != expectedNodeId)
        {
            fail("ValidateGraph - node.id does not match actual node index");
        }

        if (node.scopeId == InvalidExecScopeId ||
            node.scopeId >= graph.scopeCount)
        {
            fail("ValidateGraph - node has invalid scopeId");
        }

        if (node.phase == ExecPhase::Simulate)
            ++simulateCount;

        const uint64_t predEnd =
            static_cast<uint64_t>(node.predBegin) + static_cast<uint64_t>(node.predCount);
        const uint64_t succEnd =
            static_cast<uint64_t>(node.succBegin) + static_cast<uint64_t>(node.succCount);

        if (predEnd > graph.edges.size())
        {
            fail("ValidateGraph - node predecessor range exceeds edge pool");
        }

        if (succEnd > graph.edges.size())
        {
            fail("ValidateGraph - node successor range exceeds edge pool");
        }

        for (uint32_t i = 0; i < node.predCount; ++i)
        {
            const uint32_t edgeIndex = node.predBegin + i;
            if (edgeIndex >= graph.edges.size())
                break;

            const ExecNodeId predId = graph.edges[edgeIndex];
            if (!graph.IsValidNodeId(predId))
            {
                fail("ValidateGraph - predecessor edge references invalid node id");
                continue;
            }

            if (!policy.materializeCrossPhaseRuntimeDeps &&
                graph.nodes[predId].phase != node.phase)
            {
                fail("ValidateGraph - predecessor edge crosses phase unexpectedly");
            }
        }

        for (uint32_t i = 0; i < node.succCount; ++i)
        {
            const uint32_t edgeIndex = node.succBegin + i;
            if (edgeIndex >= graph.edges.size())
                break;

            const ExecNodeId succId = graph.edges[edgeIndex];
            if (!graph.IsValidNodeId(succId))
            {
                fail("ValidateGraph - successor edge references invalid node id");
                continue;
            }

            if (!policy.materializeCrossPhaseRuntimeDeps &&
                graph.nodes[succId].phase != node.phase)
            {
                fail("ValidateGraph - successor edge crosses phase unexpectedly");
            }
        }
    }

    if (simulateCount != graph.simulateNodeCount)
    {
        fail("ValidateGraph - simulateNodeCount does not match actual simulate node count");
    }

    // 3) pred/succ 상호 일관성
    for (uint32_t nodeId = 0;
        nodeId < static_cast<uint32_t>(graph.nodes.size());
        ++nodeId)
    {
        const ExecNodeRecord& node = graph.nodes[nodeId];

        for (uint32_t i = 0; i < node.succCount; ++i)
        {
            const uint32_t edgeIndex = node.succBegin + i;
            if (edgeIndex >= graph.edges.size())
                break;

            const ExecNodeId succId = graph.edges[edgeIndex];
            if (!graph.IsValidNodeId(succId))
                continue;

            const ExecNodeRecord& succNode = graph.nodes[succId];

            bool foundBackRef = false;
            for (uint32_t j = 0; j < succNode.predCount; ++j)
            {
                const uint32_t predEdgeIndex = succNode.predBegin + j;
                if (predEdgeIndex >= graph.edges.size())
                    break;

                if (graph.edges[predEdgeIndex] == nodeId)
                {
                    foundBackRef = true;
                    break;
                }
            }

            if (!foundBackRef)
            {
                fail("ValidateGraph - succ edge missing matching predecessor back-reference");
            }
        }
    }

    // 4) serial plan range 정합성
    auto validateRange =
        [&](const ExecRange& range, ExecPhase expectedPhase, const char* name)
        {
            const uint64_t end =
                static_cast<uint64_t>(range.begin) + static_cast<uint64_t>(range.count);

            if (end > graph.serialExecutionOrder.size())
            {
                std::string msg =
                    "ValidateGraph - serial plan range out of bounds in phase: ";
                msg += name;
                fail(msg.c_str());
                return;
            }

            for (uint32_t i = 0; i < range.count; ++i)
            {
                const uint32_t orderIndex = range.begin + i;
                const ExecNodeId nodeId = graph.serialExecutionOrder[orderIndex];

                if (!graph.IsValidNodeId(nodeId))
                {
                    std::string msg =
                        "ValidateGraph - serial order references invalid node id in phase: ";
                    msg += name;
                    fail(msg.c_str());
                    continue;
                }

                if (policy.requireSerialPhaseBucketConsistency &&
                    graph.nodes[nodeId].phase != expectedPhase)
                {
                    std::string msg =
                        "ValidateGraph - serial plan contains wrong phase node in phase: ";
                    msg += name;
                    fail(msg.c_str());
                }
            }
        };

    validateRange(graph.commitPlan, ExecPhase::Commit, "Commit");
    validateRange(graph.lifecycleFlushPlan, ExecPhase::LifecycleFlush, "LifecycleFlush");
    validateRange(graph.reconcilePlan, ExecPhase::Reconcile, "Reconcile");

    // 5) serial range 순서 / 비중첩성
    if (policy.requireContiguousSerialPlans)
    {
        const uint32_t commitEnd = graph.commitPlan.End();
        const uint32_t flushEnd = graph.lifecycleFlushPlan.End();
        const uint32_t reconcileEnd = graph.reconcilePlan.End();

        if (graph.commitPlan.begin != 0)
        {
            fail("ValidateGraph - commitPlan must begin at 0 in serialExecutionOrder");
        }

        if (graph.lifecycleFlushPlan.begin != commitEnd)
        {
            fail("ValidateGraph - lifecycleFlushPlan must begin immediately after commitPlan");
        }

        if (graph.reconcilePlan.begin != flushEnd)
        {
            fail("ValidateGraph - reconcilePlan must begin immediately after lifecycleFlushPlan");
        }

        if (reconcileEnd != graph.serialExecutionOrder.size())
        {
            fail("ValidateGraph - serialExecutionOrder has trailing or missing entries");
        }
    }

    // 6) serial topo order 유효성
    if (policy.requireSerialTopoValidity)
    {
        std::vector<int32_t> serialPosition(graph.nodes.size(), -1);

        for (uint32_t pos = 0;
            pos < static_cast<uint32_t>(graph.serialExecutionOrder.size());
            ++pos)
        {
            const ExecNodeId nodeId = graph.serialExecutionOrder[pos];
            if (graph.IsValidNodeId(nodeId))
                serialPosition[nodeId] = static_cast<int32_t>(pos);
        }

        auto validatePhaseTopo =
            [&](ExecPhase phase, const char* name)
            {
                for (uint32_t nodeId = 0;
                    nodeId < static_cast<uint32_t>(graph.nodes.size());
                    ++nodeId)
                {
                    const ExecNodeRecord& node = graph.nodes[nodeId];
                    if (node.phase != phase)
                        continue;

                    for (uint32_t i = 0; i < node.succCount; ++i)
                    {
                        const uint32_t edgeIndex = node.succBegin + i;
                        if (edgeIndex >= graph.edges.size())
                            break;

                        const ExecNodeId succId = graph.edges[edgeIndex];
                        if (!graph.IsValidNodeId(succId))
                            continue;

                        if (graph.nodes[succId].phase != phase)
                            continue;

                        if (serialPosition[nodeId] < 0 || serialPosition[succId] < 0)
                        {
                            std::string msg =
                                "ValidateGraph - serial topo validation missing node position in phase: ";
                            msg += name;
                            fail(msg.c_str());
                            continue;
                        }

                        if (serialPosition[nodeId] >= serialPosition[succId])
                        {
                            std::string msg =
                                "ValidateGraph - serialExecutionOrder violates dependency order in phase: ";
                            msg += name;
                            fail(msg.c_str());
                        }
                    }
                }
            };

        validatePhaseTopo(ExecPhase::Commit, "Commit");
        validatePhaseTopo(ExecPhase::LifecycleFlush, "LifecycleFlush");
        validatePhaseTopo(ExecPhase::Reconcile, "Reconcile");
    }

    return ok;
}

void ExecutionGraphBuilder::ApplyTransitiveReduction(FrameTaskGraph& graph) const
{
    // -----------------------------------------------------------------------
    // Transitive Reduction [spec 5.3절 단계 5]
    //
    // 각 Phase 내에서 "중간 노드를 경유해도 도달 가능한 직접 엣지"를 제거한다.
    // A → B, A → C, B → C 가 존재할 때 A → C 는 B를 거치면 도달 가능하므로 제거된다.
    //
    // 알고리즘:
    //   1) Phase별 compact 인덱스 + 위상 정렬(Kahn's)
    //   2) 역위상 순으로 reachable[u] = u에서 도달 가능한 모든 노드 집합 계산
    //   3) 엣지 u→v: u의 다른 후계자 w를 통해 v에 도달 가능하면 redundant 마킹
    //   4) 전체 edge pool 재구성 (redundant 엣지 제외)
    //
    // 비용: O(N²) — Phase당 수백 노드 이하에서 무시할 수 있는 수준이다.
    // -----------------------------------------------------------------------

    const uint32_t nodeCount = static_cast<uint32_t>(graph.nodes.size());
    if (nodeCount < 3)
        return; // 노드 3개 미만이면 transitive edge 불가

    // (fromGlobalId << 32) | toGlobalId 형식의 redundant edge 집합
    std::unordered_map<uint64_t, bool> redundantEdges;

    auto makeEdgeKey = [](ExecNodeId from, ExecNodeId to) -> uint64_t
    {
        return (static_cast<uint64_t>(from) << 32) | static_cast<uint64_t>(to);
    };

    constexpr ExecPhase kPhases[] = {
        ExecPhase::Simulate,
        ExecPhase::Commit,
        ExecPhase::LifecycleFlush,
        ExecPhase::Reconcile
    };

    for (ExecPhase phase : kPhases)
    {
        // 1) 해당 Phase의 노드 수집
        std::vector<ExecNodeId> phaseNodes;
        for (ExecNodeId id = 0; id < nodeCount; ++id)
        {
            if (graph.nodes[id].phase == phase)
                phaseNodes.push_back(id);
        }

        const uint32_t n = static_cast<uint32_t>(phaseNodes.size());
        if (n < 3)
            continue; // 3개 미만이면 transitive edge 불가

        // globalId → compact index
        std::unordered_map<ExecNodeId, uint32_t> globalToCompact;
        globalToCompact.reserve(n);
        for (uint32_t ci = 0; ci < n; ++ci)
            globalToCompact.emplace(phaseNodes[ci], ci);

        // 2) same-phase 인접 리스트(후계자) 구성 — compact index 기준
        std::vector<std::vector<uint32_t>> adj(n);
        for (uint32_t ci = 0; ci < n; ++ci)
        {
            const ExecNodeRecord& node = graph.nodes[phaseNodes[ci]];
            for (uint32_t i = 0; i < node.succCount; ++i)
            {
                const uint32_t edgeIdx = node.succBegin + i;
                if (edgeIdx >= graph.edges.size())
                    continue;

                const ExecNodeId succId = graph.edges[edgeIdx];
                const auto it = globalToCompact.find(succId);
                if (it == globalToCompact.end())
                    continue; // cross-phase 엣지 제외

                adj[ci].push_back(it->second);
            }
        }

        // 3) Kahn's 위상 정렬
        std::vector<uint32_t> indegree(n, 0);
        for (uint32_t ci = 0; ci < n; ++ci)
            for (uint32_t s : adj[ci])
                ++indegree[s];

        std::queue<uint32_t> ready;
        for (uint32_t ci = 0; ci < n; ++ci)
            if (indegree[ci] == 0)
                ready.push(ci);

        std::vector<uint32_t> topoOrder;
        topoOrder.reserve(n);
        while (!ready.empty())
        {
            const uint32_t cur = ready.front();
            ready.pop();
            topoOrder.push_back(cur);
            for (uint32_t s : adj[cur])
                if (--indegree[s] == 0)
                    ready.push(s);
        }

        // 위상 정렬이 완성되지 않으면 사이클 — 이미 ValidateFragmentAcyclicPerPhase가
        // 걸러야 하므로 여기서는 안전하게 건너뛴다.
        if (topoOrder.size() != n)
            continue;

        // 4) 역위상 순으로 reachable 집합 계산
        // reachable[ci] = ci에서 직/간접적으로 도달 가능한 compact index 집합
        // unordered_set 대신 vector<bool>(bitset) 사용 — 캐시 효율 + 작은 n에 최적
        std::vector<std::vector<bool>> reachable(n, std::vector<bool>(n, false));
        for (int32_t ti = static_cast<int32_t>(topoOrder.size()) - 1; ti >= 0; --ti)
        {
            const uint32_t ci = topoOrder[ti];
            for (uint32_t s : adj[ci])
            {
                reachable[ci][s] = true;
                // s에서 도달 가능한 노드를 ci에도 전파
                for (uint32_t k = 0; k < n; ++k)
                {
                    if (reachable[s][k])
                        reachable[ci][k] = true;
                }
            }
        }

        // 5) 각 엣지 ci→v 검사: ci의 다른 후계자 w를 통해 v에 도달 가능하면 redundant
        for (uint32_t ci = 0; ci < n; ++ci)
        {
            for (uint32_t v : adj[ci])
            {
                // ci의 다른 후계자 w를 통해 v에 도달 가능한지 확인
                for (uint32_t w : adj[ci])
                {
                    if (w == v)
                        continue;

                    if (reachable[w][v])
                    {
                        // ci → v 는 transitive — redundant 마킹
                        redundantEdges.emplace(
                            makeEdgeKey(phaseNodes[ci], phaseNodes[v]), true);
                        break;
                    }
                }
            }
        }
    }

    if (redundantEdges.empty())
        return;

    // 6) 전체 edge pool 재구성
    // 모든 노드의 pred/succ 목록을 vector<vector>로 추출 후 redundant 제거
    std::vector<std::vector<ExecNodeId>> newPreds(nodeCount);
    std::vector<std::vector<ExecNodeId>> newSuccs(nodeCount);

    for (ExecNodeId id = 0; id < nodeCount; ++id)
    {
        const ExecNodeRecord& node = graph.nodes[id];

        for (uint32_t i = 0; i < node.predCount; ++i)
        {
            const uint32_t edgeIdx = node.predBegin + i;
            if (edgeIdx >= graph.edges.size())
                continue;

            const ExecNodeId predId = graph.edges[edgeIdx];
            // predId → id 방향 키
            if (redundantEdges.count(makeEdgeKey(predId, id)) == 0)
                newPreds[id].push_back(predId);
        }

        for (uint32_t i = 0; i < node.succCount; ++i)
        {
            const uint32_t edgeIdx = node.succBegin + i;
            if (edgeIdx >= graph.edges.size())
                continue;

            const ExecNodeId succId = graph.edges[edgeIdx];
            // id → succId 방향 키
            if (redundantEdges.count(makeEdgeKey(id, succId)) == 0)
                newSuccs[id].push_back(succId);
        }
    }

    // edge pool 전체 교체
    graph.edges.clear();
    for (ExecNodeId id = 0; id < nodeCount; ++id)
    {
        ExecNodeRecord& node = graph.nodes[id];

        node.predBegin = static_cast<uint32_t>(graph.edges.size());
        node.predCount = static_cast<uint32_t>(newPreds[id].size());
        for (ExecNodeId pred : newPreds[id])
            graph.edges.push_back(pred);

        node.succBegin = static_cast<uint32_t>(graph.edges.size());
        node.succCount = static_cast<uint32_t>(newSuccs[id].size());
        for (ExecNodeId succ : newSuccs[id])
            graph.edges.push_back(succ);
    }
}

#if defined(_DEBUG)
void ExecutionGraphBuilder::WriteDebugGraph(
    const FrameTaskGraph& graph,
    const ExecutionSourceRegistry& sourceRegistry) const
{
    auto phaseName = [](ExecPhase phase) -> const char*
        {
            switch (phase)
            {
            case ExecPhase::Simulate: return "Simulate";
            case ExecPhase::Commit: return "Commit";
            case ExecPhase::LifecycleFlush: return "LifecycleFlush";
            case ExecPhase::Reconcile: return "Reconcile";
            default: return "None";
            }
        };

    auto laneName = [](ExecLane lane) -> const char*
        {
            switch (lane)
            {
            case ExecLane::Parallel: return "Parallel";
            case ExecLane::Serial: return "Serial";
            case ExecLane::Main: return "Main";
            default: return "None";
            }
        };

    auto kindName = [](ExecNodeKind kind) -> const char*
        {
            switch (kind)
            {
            case ExecNodeKind::StaticSystem: return "StaticSystem";
            case ExecNodeKind::DynamicTask: return "DynamicTask";
            case ExecNodeKind::StructuralApply: return "StructuralApply";
            case ExecNodeKind::DeferredStateApply: return "DeferredStateApply";
            case ExecNodeKind::PostCommitFinalize: return "PostCommitFinalize";
            case ExecNodeKind::LifecycleFlush: return "LifecycleFlush";
            case ExecNodeKind::Reconcile: return "Reconcile";
            default: return "None";
            }
        };

    auto escapeDot = [](const std::string& value) -> std::string
        {
            std::string escaped;
            escaped.reserve(value.size());

            for (const char ch : value)
            {
                switch (ch)
                {
                case '\\':
                    escaped += "\\\\";
                    break;
                case '"':
                    escaped += "\\\"";
                    break;
                case '\n':
                    escaped += "\\n";
                    break;
                case '\r':
                    break;
                default:
                    escaped += ch;
                    break;
                }
            }

            return escaped;
        };

    auto sourceName = [&](ExecToken token) -> std::string
        {
            const ExecutionSourceDesc* desc = sourceRegistry.TryGet(token);
            if (desc == nullptr || desc->debugName.empty())
                return "token_" + std::to_string(token);

            return desc->debugName;
        };

    std::ofstream out{ "frame_graph_debug.dot", std::ios::out | std::ios::trunc };
    if (!out.is_open())
        return;

    out << "digraph FrameTaskGraph {\n";
    out << "  graph [rankdir=LR, labelloc=\"t\", label=\"FrameTaskGraph\"];\n";
    out << "  node [shape=box, fontname=\"Consolas\", fontsize=10];\n";
    out << "  edge [fontname=\"Consolas\", fontsize=9];\n\n";

    out << "  // scopes=" << graph.scopeCount
        << ", nodes=" << graph.nodes.size()
        << ", simulateNodes=" << graph.simulateNodeCount
        << ", edgePoolEntries=" << graph.edges.size() << "\n\n";

    for (const ExecNodeRecord& node : graph.nodes)
    {
        std::ostringstream label;
        label
            << "#" << node.id
            << "\\n" << sourceName(node.sourceToken)
            << "\\nphase=" << phaseName(node.phase)
            << "\\nlane=" << laneName(node.lane)
            << "\\nkind=" << kindName(node.kind)
            << "\\nscope=" << node.scopeId
            << "\\ntoken=" << node.sourceToken
            << "\\npred=" << node.predCount
            << " succ=" << node.succCount;

        out << "  n" << node.id
            << " [label=\"" << escapeDot(label.str()) << "\"];\n";
    }

    out << "\n";

    for (const ExecNodeRecord& node : graph.nodes)
    {
        for (uint32_t i = 0; i < node.succCount; ++i)
        {
            const uint32_t edgeIndex = node.succBegin + i;
            if (edgeIndex >= graph.edges.size())
                continue;

            const ExecNodeId succId = graph.edges[edgeIndex];
            if (!graph.IsValidNodeId(succId))
                continue;

            out << "  n" << node.id << " -> n" << succId << ";\n";
        }
    }

    auto writePlan = [&](const char* name, const ExecRange& range)
        {
            out << "\n  // " << name << " begin=" << range.begin
                << " count=" << range.count << "\n";

            for (uint32_t i = 0; i < range.count; ++i)
            {
                const uint32_t orderIndex = range.begin + i;
                if (orderIndex >= graph.serialExecutionOrder.size())
                    break;

                out << "  //   [" << i << "] node="
                    << graph.serialExecutionOrder[orderIndex] << "\n";
            }
        };

    writePlan("commitPlan", graph.commitPlan);
    writePlan("lifecycleFlushPlan", graph.lifecycleFlushPlan);
    writePlan("reconcilePlan", graph.reconcilePlan);

    out << "}\n";
}
#endif

void ExecutionGraphBuilder::ReportByPolicy(BuildResult& result, BuildDecision decision, const char* message) const
{
    switch (decision) {
    case BuildDecision::Allow:
    {
        break;
    }
    case BuildDecision::Warn:
    {
        AddWarning(result, message);
        break;
    }
    case BuildDecision::Error:
    default:
    {
        AddError(result, message);
        break;
    }
    }
}

void ExecutionGraphBuilder::AddError(BuildResult& result, const char* message) const
{
    result.diagnostics.push_back(
        BuildDiagnostic{
            BuildDiagnosticSeverity::Error,
            (message != nullptr) ? message : "ExecutionGraphBuilder - unknown error"
        });
}

void ExecutionGraphBuilder::AddWarning(BuildResult& result, const char* message) const
{
    result.diagnostics.push_back(
        BuildDiagnostic{
            BuildDiagnosticSeverity::Warning,
            (message != nullptr) ? message : "ExecutionGraphBuilder - unknown warning"
        });
}

