#pragma once

#include <cstdint>

enum class BuildDecision : uint8_t { Allow, Warn, Error };

struct ExecutionGraphBuildPolicy
{
    // 1. 입력/구성 정책
    BuildDecision emptyFrameSelection               = BuildDecision::Warn;
    BuildDecision invalidSelection                  = BuildDecision::Error;
    BuildDecision missingExecutionModel             = BuildDecision::Error;
    BuildDecision invalidExecutionModel             = BuildDecision::Error;
    BuildDecision missingExecutionSource            = BuildDecision::Error;
    BuildDecision invalidExecutionSource            = BuildDecision::Error;
    BuildDecision sourcePhaseMismatch               = BuildDecision::Error;

    // 2. dependency / graph validation 정책
    BuildDecision duplicateSourceTokenInModel       = BuildDecision::Error;
    BuildDecision invalidDependencyEdge             = BuildDecision::Error;
    BuildDecision selfDependencyEdge                = BuildDecision::Error;
    BuildDecision edgeOutsideModel                  = BuildDecision::Error;
    BuildDecision backwardPhaseDependency           = BuildDecision::Error;
    BuildDecision crossPhaseExplicitEdgeDisallowed  = BuildDecision::Error;
    BuildDecision samePhaseCycle                    = BuildDecision::Error;

    // 3. graph shaping 정책
    bool requireDenseScopeIds                   = true;
    bool deduplicateSamePhaseEdges              = true;
    bool allowCrossPhaseExplicitEdges           = true;
    bool materializeCrossPhaseRuntimeDeps       = false;

    // Transitive Reduction [spec 5.3절 단계 5]:
    // 중간 노드를 경유해도 도달 가능한 직접 엣지를 제거한다.
    // 병렬 스케줄러의 false dependency를 줄여 동시 실행 기회를 최대화한다.
    // 비용: O(Phase 노드 수²) — 수백 개 System 수준에서는 무시할 수 있다.
    bool applyTransitiveReduction               = true;
    // bool deduplicateCrossPhaseEdges

    // 4. serial plan 정책
    bool requireContiguousSerialPlans           = true;
    bool requireSerialPhaseBucketConsistency    = true;
    bool requireSerialTopoValidity              = true;

    // 5. debug / strictness
    // bool keepInvalidGraphForDiagnostics         = true;
    // bool strictValidation                       = true;
};