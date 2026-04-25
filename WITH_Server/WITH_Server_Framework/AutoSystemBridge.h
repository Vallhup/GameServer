#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "SystemManager.h"
#include "ExecutionSourceTypes.h"
#include "WorldExecutionModelTypes.h"

class WorldRuntime;

// ---------------------------------------------------------------------------
// AutoSystemBridge
//
// SystemManager에 등록된 시스템을 TaskGraph 실행 파이프라인에 자동으로 연결한다.
//
// 역할:
//   1. SystemManager::BuildScheduleDescs()로 시스템 메타 수집
//   2. ExecutionSourceRegistry에 ExecutionSourceDesc 등록
//      - token   : AllocateToken()으로 발급
//      - fn      : BridgeDispatchFn (System::Execute 위임 래퍼)
//      - accesses: ExecMeta.accesses span을 그대로 전달
//   3. WorldRuntime 디스패치 테이블에 token → System* 등록
//   4. WorldExecutionModel의 해당 Phase 소스 목록에 token 추가
//   5. ExecMeta.runsBefore / runsAfter → ExplicitEdge 변환
//
// 단계 Phase 매핑:
//   SystemPhase::Pre   → ExecPhase::Reconcile  (사전 정리)
//   SystemPhase::Graph → ExecPhase::Simulate   (메인 병렬 시뮬레이션)
//   SystemPhase::Post  → ExecPhase::Commit     (후처리)
//
// 주의:
//   - Bridge() 호출 전에 registry.AllocateToken()이 아직 초기화되어 있어야 한다.
//   - 동일 WorldExecutionModel에 대해 Bridge()를 두 번 호출하면 중복 등록된다.
//     호출자는 Clear() / 새 Model을 사용해야 한다.
// ---------------------------------------------------------------------------
class AutoSystemBridge {
public:
    struct BridgeResult
    {
        bool success{ false };
        int  registeredCount{ 0 };
        std::string errorMessage;
    };

    // systemPhase: SystemManager에서 꺼낼 Phase
    // execPhase  : WorldExecutionModel에 등록할 ExecPhase
    BridgeResult Bridge(
        SystemPhase               systemPhase,
        ExecPhase                 execPhase,
        SystemManager&            systemManager,
        ExecutionSourceRegistry&  sourceRegistry,
        WorldRuntime&             worldRuntime,
        WorldExecutionModel&      outModel);

    BridgeResult RegisterSources(
        SystemPhase               systemPhase,
        ExecPhase                 execPhase,
        SystemManager&            systemManager,
        ExecutionSourceRegistry&  sourceRegistry);

    BridgeResult BuildModelFromRegisteredSources(
        SystemPhase                    systemPhase,
        ExecPhase                      execPhase,
        SystemManager&                 systemManager,
        const ExecutionSourceRegistry& sourceRegistry,
        WorldExecutionModel&           outModel);

    BridgeResult BindRuntimeDispatch(
        SystemPhase                    systemPhase,
        ExecPhase                      execPhase,
        SystemManager&                 systemManager,
        const ExecutionSourceRegistry& sourceRegistry,
        WorldRuntime&                  worldRuntime);

private:
    // BridgeDispatchFn — ExecFn 시그니처와 일치하는 정적 함수.
    // NodeExecContext.sourceToken으로 WorldRuntime에서 System*을 조회해 Execute를 호출한다.
    static ExecCallResult BridgeDispatch(NodeExecContext& ctx);

    // ExecPhase → ExecNodeKind 매핑 헬퍼
    static ExecNodeKind PhaseToNodeKind(ExecPhase phase) noexcept;

    // 두 번째 패스: runsBefore / runsAfter → explicitEdge 변환.
    // tagToToken 은 ExecTag → ExecToken 매핑 (브릿지 1패스에서 수집).
    static void ApplyOrderingHints(
        const std::vector<SystemScheduleDesc>& descs,
        const std::vector<ExecToken>&          tokens,
        WorldExecutionModel&                   outModel);

    static const ExecutionSourceDesc* FindRegisteredSource(
        const ExecutionSourceRegistry& sourceRegistry,
        ExecPhase                      execPhase,
        std::string_view               debugName) noexcept;
};
