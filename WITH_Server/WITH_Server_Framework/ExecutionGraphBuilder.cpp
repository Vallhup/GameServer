#include "pch.h"
#include "ExecutionGraphBuilder.h"

#include <algorithm>
#include <queue>
#include <string>
#include <utility>
#include <deque>
#include <unordered_map>

#include "ExecutionCoreTypes.h"
#include "ExecutionSourceTypes.h"

#include "WorldFragmentBuild.h"
#include "WorldExecutionModelTypes.h"

BuildResult ExecutionGraphBuilder::Build(const FrameBuildContext& context)
{
    BuildResult result{};
    result.graph.Clear();
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

    AssembleFrameGraph(fragments, result.graph, policy);
    BuildSerialExecutionPlan(result.graph, policy);

    if (!ValidateGraph(result.graph, policy, result))
    {
        result.success = !result.HasError();
        return result;
    }

    result.success = !result.HasError();
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
        if (!BuildSingleWorldFragment(selection, *model, sourceRegistry, policy, fragment, result))
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

        const uint32_t localIndex =
            static_cast<uint32_t>(outFragment.localNodes.size());

        outFragment.localNodes.push_back(node);
        tokenToLocalIndex.try_emplace(desc->token, localIndex);
    }

    if (!BuildFragmentEdges(model, tokenToLocalIndex, policy, outFragment, result))
        return false;

    if (!ValidateFragmentAcyclicPerPhase(outFragment, policy, result))
        return false;

    return true;
}

bool ExecutionGraphBuilder::BuildFragmentEdges(
    const WorldExecutionModel& model,
    const std::unordered_map<ExecToken, uint32_t>& tokenToLocalIndex,
    const ExecutionGraphBuildPolicy& policy,
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

void ExecutionGraphBuilder::AssembleFrameGraph(
    const std::vector<WorldFragmentBuild>& fragments,
    FrameTaskGraph& outGraph,
    const ExecutionGraphBuildPolicy& policy) const
{
    outGraph.Clear();

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
    uint32_t totalEdgeCount = 0;

    for (const WorldFragmentBuild& fragment : fragments)
    {
        totalNodeCount += static_cast<uint32_t>(fragment.localNodes.size());
        totalEdgeCount += static_cast<uint32_t>(fragment.localEdges.size());
    }

    outGraph.nodes.reserve(totalNodeCount);
    outGraph.edges.reserve(static_cast<size_t>(totalEdgeCount) * 2);

    std::vector<std::vector<ExecNodeId>> predLists(totalNodeCount);
    std::vector<std::vector<ExecNodeId>> succLists(totalNodeCount);

    struct FragmentRemap
    {
        const WorldFragmentBuild* fragment{ nullptr };
        std::vector<ExecNodeId> localToGlobal;
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
            const ExecNodeId toGlobal = remap.localToGlobal[edge.toLocalIndex];

            if (fromGlobal == InvalidExecNodeId || toGlobal == InvalidExecNodeId)
                continue;

            const ExecPhase fromPhase = outGraph.nodes[fromGlobal].phase;
            const ExecPhase toPhase = outGraph.nodes[toGlobal].phase;

            if (fromPhase != toPhase && !policy.materializeCrossPhaseRuntimeDeps)
                continue;

            appendUnique(succLists[fromGlobal], toGlobal);
            appendUnique(predLists[toGlobal], fromGlobal);
        }
    }

    for (ExecNodeId nodeId = 0;
        nodeId < static_cast<ExecNodeId>(outGraph.nodes.size());
        ++nodeId)
    {
        ExecNodeRecord& node = outGraph.nodes[nodeId];

        node.predBegin = static_cast<uint32_t>(outGraph.edges.size());
        node.predCount = static_cast<uint32_t>(predLists[nodeId].size());
        for (ExecNodeId pred : predLists[nodeId])
            outGraph.edges.push_back(pred);

        node.succBegin = static_cast<uint32_t>(outGraph.edges.size());
        node.succCount = static_cast<uint32_t>(succLists[nodeId].size());
        for (ExecNodeId succ : succLists[nodeId])
            outGraph.edges.push_back(succ);
    }
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

