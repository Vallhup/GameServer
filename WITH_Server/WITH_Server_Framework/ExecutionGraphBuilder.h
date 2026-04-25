#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "ExecutionContextTypes.h"
#include "ExecutionGraphTypes.h"
#include "ExecutionGraphBuildPolicy.h"
#include "ConflictDetection.h"

struct WorldFragmentBuild;
struct WorldFrameSelection;
struct WorldExecutionModel;

// NOTE:
// BuildDecision controls diagnostic severity only.
// Fragment viability is decided by builder control flow.
// Missing/invalid source/model/dependency invalidates the fragment
// even when the diagnostic severity is Warn.

class ExecutionGraphBuilder final {
public:
    BuildResult Build(const FrameBuildContext& context);

private:
    bool ValidateBuildContext(
        const FrameBuildContext& context,
        BuildResult& result
    ) const;

    bool BuildWorldFragments(
        const FrameBuildContext& context,
        std::vector<WorldFragmentBuild>& outFragments,
        BuildResult& result
    ) const;

    bool BuildSingleWorldFragment(
        const WorldFrameSelection& selection,
        const WorldExecutionModel& model,
        const ExecutionSourceRegistry& sourceRegistry,
        const ExecutionGraphBuildPolicy& policy,
        const ConflictRegistry* conflictRegistry,
        WorldFragmentBuild& outFragment,
        BuildResult& result
    ) const;

    bool BuildFragmentEdges(
        const WorldExecutionModel& model,
        const ExecutionSourceRegistry& sourceRegistry,
        const std::unordered_map<ExecToken, uint32_t>& tokenToLocalIndex,
        const ExecutionGraphBuildPolicy& policy,
        const ConflictRegistry* conflictRegistry,
        WorldFragmentBuild& outFragment,
        BuildResult& result
    ) const;

    bool ValidateFragmentAcyclicPerPhase(
        const WorldFragmentBuild& fragment,
        const ExecutionGraphBuildPolicy& policy,
        BuildResult& result
    ) const;

    void AssembleFrameGraph(
        const std::vector<WorldFragmentBuild>& fragments,
        FrameTaskGraph& outGraph,
        const ExecutionGraphBuildPolicy& policy
    ) const;

    // Transitive Reduction [spec 5.3절 단계 5]
    // AssembleFrameGraph 완료 후, BuildSerialExecutionPlan 이전에 호출한다.
    // Phase별로 중간 경유 노드가 존재하는 직접 엣지를 제거하여
    // 병렬 스케줄러의 동시 실행 기회를 최대화한다.
    void ApplyTransitiveReduction(FrameTaskGraph& graph) const;

    void BuildSerialExecutionPlan(
        FrameTaskGraph& graph,
        const ExecutionGraphBuildPolicy& policy
    ) const;

    bool ValidateGraph(
        const FrameTaskGraph& graph,
        const ExecutionGraphBuildPolicy& policy,
        BuildResult& result
    ) const;

    void ReportByPolicy(
        BuildResult& result,
        BuildDecision decision,
        const char* message
    ) const;

    void AddError(BuildResult& result, const char* message) const;
    void AddWarning(BuildResult& result, const char* message) const;

#if defined(_DEBUG)
    // Debug 빌드 전용 — 프레임 그래프를 DOT 형식 파일로 기록한다.
    // 경로: "<workingDir>/frame_graph_debug.dot"
    // Build() 성공 시 자동 호출된다.
    void WriteDebugGraph(
        const FrameTaskGraph& graph,
        const ExecutionSourceRegistry& sourceRegistry
    ) const;
#endif
};