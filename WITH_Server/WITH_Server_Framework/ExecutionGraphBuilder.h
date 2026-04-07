#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "ExecutionContextTypes.h"
#include "ExecutionGraphTypes.h"
#include "ExecutionGraphBuildPolicy.h"

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
        WorldFragmentBuild& outFragment,
        BuildResult& result
    ) const;

    bool BuildFragmentEdges(
        const WorldExecutionModel& model,
        const std::unordered_map<ExecToken, uint32_t>& tokenToLocalIndex,
        const ExecutionGraphBuildPolicy& policy,
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
};