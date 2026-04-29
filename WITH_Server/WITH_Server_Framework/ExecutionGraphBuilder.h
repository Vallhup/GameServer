#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "ExecutionContextTypes.h"
#include "ExecutionGraphTypes.h"
#include "ExecutionGraphBuildPolicy.h"
#include "ConflictDetection.h"
#include "DynamicTaskTypes.h"

struct WorldFragmentBuild;
struct WorldFrameSelection;
struct WorldExecutionModel;

// ---------------------------------------------------------------------------
// 화이트박스 테스트 전용 훅 — 프로덕션 코드에서는 사용하지 않는다.
// DynamicTask_SmokeTest.cpp 에서만 정의된다.
// ---------------------------------------------------------------------------
struct ExecutionGraphBuilderTestHook;

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

    // [1/3] 정적 fragments로부터 노드를 전역 그래프에 배치하고
    //       pred/succ 리스트를 out 파라미터로 반환한다.
    //       FinalizeEdgePool() 호출 전까지 graph.edges는 비어 있다.
    void AssembleFrameGraphNodes(
        const std::vector<WorldFragmentBuild>& fragments,
        FrameTaskGraph& outGraph,
        const ExecutionGraphBuildPolicy& policy,
        std::vector<std::vector<ExecNodeId>>& outPredLists,
        std::vector<std::vector<ExecNodeId>>& outSuccLists
    ) const;

    // [2/3] 동결된 DynamicTask 요청을 그래프에 추가한다.
    //       AssembleFrameGraphNodes() 이후, FinalizeEdgePool() 이전에 호출한다.
    //       outFrameTable에 각 인스턴스를 등록한다.
    void AppendDynamicNodes(
        const DynamicTaskFrozenBatch& batch,
        const DynamicTaskTypeRegistry& typeRegistry,
        const ExecutionSourceRegistry& sourceRegistry,
        const ConflictRegistry* conflictRegistry,
        FrameTaskGraph& outGraph,
        std::vector<std::vector<ExecNodeId>>& predLists,
        std::vector<std::vector<ExecNodeId>>& succLists,
        DynamicTaskFrameTable& outFrameTable,
        BuildResult& result
    ) const;

    // [3/3] predLists / succLists를 graph.edges에 직렬화하고 각 노드의
    //       predBegin/predCount/succBegin/succCount를 확정한다.
    void FinalizeEdgePool(
        FrameTaskGraph& outGraph,
        const std::vector<std::vector<ExecNodeId>>& predLists,
        const std::vector<std::vector<ExecNodeId>>& succLists,
        const ExecutionGraphBuildPolicy& policy
    ) const;

    // [레거시 단일 호출 래퍼] dynamic batch가 없는 경우에 사용한다.
    // AssembleFrameGraphNodes → FinalizeEdgePool 을 묶은 편의 함수.
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

    // 화이트박스 테스트 훅에게만 private 접근을 허용한다.
    friend struct ExecutionGraphBuilderTestHook;

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