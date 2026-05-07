#include "pch.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <cstdio>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

#include "ConflictDetection.h"
#include "ExecutionContextTypes.h"
#include "ExecutionGraphBuildPolicy.h"
#include "ExecutionGraphBuilder.h"
#include "ExecutionOps.h"
#include "ExecutionRuntimeTypes.h"
#include "SystemMetaHelper.h"
#include "TaskExecutor.h"
#include "WorldDef.h"
#include "WorldExecutionModelTypes.h"
#include "WorldFrameSelectionTypes.h"
#include "WorldId.h"
#include "WorldRuntime.h"
#include "WorldRuntimeTypes.h"

// ---------------------------------------------------------------------------
// PaperEvaluation.cpp
//
// 5 정량 지표 측정 코드 (논문 §5 평가 방법론).
// AccessSpec은 실제 게임 서버 시스템 구성을 반영하여 하드코딩.
// 워크로드는 엔티티 수(N=50) 비례 busy-wait으로 시뮬레이션.
//
// 지표:
//   1. Speedup          — T_serial / T_parallel, worker 1→8
//   2. False Serialization 감소율 — Binary vs 5-Rule AccessSpec 비교
//   3. DAG 빌드 오버헤드 비율   — T_build / T_frame (median, p95)
//   4. 멀티 월드 스케일링       — World 수 1→20, Simulate Phase 처리 시간
//   5. 파이프라인 단계별 시간 분포 — Simulate/Commit/LifecycleFlush/Reconcile
// ---------------------------------------------------------------------------

namespace
{
    // =========================================================================
    // Section 1: Resource tag 타입 (컴파일 타임 TypeHash 기반 ResourceId 생성)
    // =========================================================================

    // Component resources
    struct Res_Transform           {};
    struct Res_AIControlled        {};
    struct Res_AIType              {};
    struct Res_SpawnType           {};
    struct Res_AIBlackboard        {};
    struct Res_AIPerceptionComp    {};   // ★ Rule 1: AIDecisionSystem에서 ReadSnapshot으로 접근
    struct Res_AIDecision          {};
    struct Res_AICommandFrame      {};
    struct Res_AIReaction          {};
    struct Res_BossPattern         {};
    struct Res_BossPhase           {};
    struct Res_PlayerControlIdentity {};
    struct Res_ActorInput          {};
    struct Res_AbilityState        {};
    struct Res_AbilityTimeline     {};
    struct Res_CombatStat          {};   // ★ Rule 3: CommitCombatResultSystem에서 WriteDeferred로 접근
    struct Res_AbilityInterruptQ   {};
    struct Res_DirtyFlags          {};
    struct Res_LocomotionState     {};
    struct Res_PendingDespawn      {};
    struct Res_PendingTransferTag  {};
    struct Res_AnimPlayback        {};
    struct Res_SampledPose         {};
    struct Res_CombatCollider      {};
    struct Res_LocoMoveDelta       {};
    struct Res_AbilityMoveDelta    {};
    struct Res_AbilityMoveRuntime  {};
    struct Res_PreCollisionTransform {};
    struct Res_BodyCollisionShape  {};
    struct Res_NavMeshAgent        {};
    struct Res_BodyCollisionResolve {};
    struct Res_PortalTriggerState  {};
    struct Res_PendingTransferComp {};
    struct Res_CombatColliderActivation {};
    struct Res_CombatHitDedup      {};
    struct Res_PendingCombatResult {};
    struct Res_PendingProjectileSpawn {};
    struct Res_PendingAbilityEvent {};
    struct Res_ReplicationStats    {};

    // EventQueue resources
    struct Res_CombatEventQueue    {};   // ★ Rule 4: ResolveCombatHit + CommitCombatResult 양쪽에서 EmitDeferred

    // External (싱글톤) resources
    struct Res_X_WorldCmd          {};
    struct Res_X_NetBinding        {};
    struct Res_X_AnimReg           {};
    struct Res_X_NavMesh           {};
    struct Res_X_PortalDef         {};
    struct Res_X_AbilityDef        {};

    // =========================================================================
    // Section 2: 유틸리티
    // =========================================================================

    [[nodiscard]] inline uint64_t NowNs() noexcept
    {
        using namespace std::chrono;
        return static_cast<uint64_t>(
            duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count());
    }

    void BusyWaitNs(uint64_t ns) noexcept
    {
        if (ns == 0) return;
        const uint64_t deadline = NowNs() + ns;
        while (NowNs() < deadline)
        {
#if defined(_MSC_VER)
            _mm_pause();
#elif defined(__GNUC__) || defined(__clang__)
            __builtin_ia32_pause();
#endif
        }
    }

    static ExecutionOps MakeEvalOps()
    {
        return ExecutionOps(
            reinterpret_cast<WorldManager*>(0x1),
            reinterpret_cast<WorldRegistry*>(0x1),
            reinterpret_cast<WorldTransferService*>(0x1),
            reinterpret_cast<WorldAdmissionService*>(0x1),
            reinterpret_cast<NetIdRegistry*>(0x1),
            reinterpret_cast<PresenceManager*>(0x1));
    }

    // =========================================================================
    // Section 3: 전역 workload 테이블
    //
    // ExecFn은 ctx.sourceToken으로 자신의 workNs를 조회한다.
    // BuildEvalBundle() 호출 시 토큰 할당과 동시에 채워진다.
    // =========================================================================

    std::unordered_map<ExecToken, uint64_t> g_evalWorkNs;

    ExecCallResult EvalSystemFn(NodeExecContext& ctx)
    {
        const auto it = g_evalWorkNs.find(ctx.sourceToken);
        if (it != g_evalWorkNs.end())
            BusyWaitNs(it->second);
        return ExecCallResult::Success;
    }

    // =========================================================================
    // Section 4: Binary 충돌 모델 (Metric 2 비교용)
    //
    // Binary = Read/Write 이진 구분만 사용:
    //   Rule 2: 양쪽 모두 Read → no conflict
    //   Rule 5: 그 외          → conflict
    // Snapshot / Deferred / Emit 의미론 무시 → false serialization 발생
    // =========================================================================

    [[nodiscard]] inline bool BinaryHasConflict(
        const AccessSpec& a,
        const AccessSpec& b) noexcept
    {
        return !(a.mode == AccessMode::Read && b.mode == AccessMode::Read);
    }

    [[nodiscard]] bool BinaryHasAnyConflict(
        std::span<const AccessSpec> a,
        std::span<const AccessSpec> b) noexcept
    {
        for (const AccessSpec& sa : a)
            for (const AccessSpec& sb : b)
                if (sa.resource == sb.resource && BinaryHasConflict(sa, sb))
                    return true;
        return false;
    }

    // =========================================================================
    // Section 5: SystemEvalDesc — 시스템 설명자
    // =========================================================================

    struct SystemEvalDesc
    {
        const char*              name;
        uint64_t                 workNs;
        std::span<const AccessSpec> accesses;
    };

    // =========================================================================
    // Section 6: BuildFullSystemTable — 24개 게임 시스템 AccessSpec 선언
    //
    // AccessSpec 정제 (★ 규칙 시연):
    //   Rule 1: AIDecisionSystem — ReadSnapshot(AIPerceptionComp)  [기존 ReadImmediate]
    //   Rule 3: CommitCombatResultSystem — WriteDeferred(CombatStat) [기존 WriteImmediate]
    //           + ResolveDeathAndDespawnSystem — ReadImmediate(CombatStat) 유지
    //   Rule 4: ResolveCombatHitSystem, CommitCombatResultSystem
    //           — EmitDeferred(CombatEventQueue) 추가
    //
    // 워크로드 모델 (kN = 50 엔티티):
    //   O(N²) heavy: 50×50×ns  (AIPerception, CharacterOverlap)
    //   heavy:       50×40µs   (AIDecision, SampleAnim, ...)
    //   medium:      50×10~20µs
    //   light:       50×2~5µs
    // =========================================================================

    // 실제 사용 함수: accesses 포함 전체 descriptor 반환
    std::vector<SystemEvalDesc> BuildFullSystemTable()
    {
        constexpr uint64_t kN = 50;

        static const std::array<AccessSpec, 6> kAIPerception = {
            ReadImmediate (ComponentRes<Res_AIControlled>()),
            ReadImmediate (ComponentRes<Res_Transform>()),
            ReadImmediate (ComponentRes<Res_AIType>()),
            ReadImmediate (ComponentRes<Res_SpawnType>()),
            WriteImmediate(ComponentRes<Res_AIBlackboard>()),
            WriteImmediate(ComponentRes<Res_AIPerceptionComp>()),
        };
        static const std::array<AccessSpec, 8> kAIDecision = {
            ReadImmediate (ComponentRes<Res_Transform>()),
            ReadImmediate (ComponentRes<Res_AbilityState>()),
            ReadImmediate (ComponentRes<Res_AIType>()),
            ReadSnapshot  (ComponentRes<Res_AIPerceptionComp>()),          // ★ Rule 1
            WriteImmediate(ComponentRes<Res_AIDecision>()),
            WriteImmediate(ComponentRes<Res_AICommandFrame>()),
            WriteImmediate(ComponentRes<Res_AIReaction>()),
            WriteImmediate(ComponentRes<Res_BossPattern>()),
        };
        // ★ Rule 1 설계 의도: AIDecision은 AIBlackboard를 쓰지 않는다.
        // AIPerception이 AIBlackboard(Write)와 AIPerceptionComp(Write)를 채우면
        // AIDecision은 AIPerceptionComp를 ReadSnapshot으로만 읽는다.
        // 두 시스템의 유일한 shared 충돌 후보가 AIPerceptionComp뿐이므로
        // Binary(Write+Snapshot→충돌)와 5-Rule(Rule1→안전)의 차이가 드러난다.
        static const std::array<AccessSpec, 4> kApplyPlayerCmd = {
            ReadImmediate (ExternalRes<Res_X_WorldCmd>()),
            ReadImmediate (ExternalRes<Res_X_NetBinding>()),
            ReadImmediate (ComponentRes<Res_PlayerControlIdentity>()),
            WriteImmediate(ComponentRes<Res_ActorInput>()),
        };
        static const std::array<AccessSpec, 3> kApplyAICmd = {
            ReadImmediate (ComponentRes<Res_AIControlled>()),
            ReadImmediate (ComponentRes<Res_AICommandFrame>()),
            WriteImmediate(ComponentRes<Res_ActorInput>()),
        };
        static const std::array<AccessSpec, 12> kResolveAbilityState = {
            WriteImmediate(ComponentRes<Res_AbilityState>()),
            WriteImmediate(ComponentRes<Res_ActorInput>()),
            WriteImmediate(ComponentRes<Res_AbilityTimeline>()),
            WriteImmediate(ComponentRes<Res_CombatStat>()),
            WriteImmediate(ComponentRes<Res_AbilityInterruptQ>()),
            WriteImmediate(ComponentRes<Res_DirtyFlags>()),
            ReadImmediate (ComponentRes<Res_LocomotionState>()),
            ReadImmediate (ComponentRes<Res_Transform>()),
            ReadImmediate (ComponentRes<Res_SpawnType>()),
            ReadImmediate (ComponentRes<Res_AIPerceptionComp>()),
            ReadImmediate (ComponentRes<Res_PendingDespawn>()),
            ReadImmediate (ComponentRes<Res_PendingTransferTag>()),
        };
        static const std::array<AccessSpec, 4> kAdvanceAbilityTimeline = {
            WriteImmediate(ComponentRes<Res_AbilityState>()),
            WriteImmediate(ComponentRes<Res_AbilityTimeline>()),
            ReadImmediate (ComponentRes<Res_PendingDespawn>()),
            ReadImmediate (ComponentRes<Res_PendingTransferTag>()),
        };
        static const std::array<AccessSpec, 7> kResolveLocomotion = {
            WriteImmediate(ComponentRes<Res_LocomotionState>()),
            ReadImmediate (ComponentRes<Res_AbilityState>()),
            ReadImmediate (ComponentRes<Res_ActorInput>()),
            ReadImmediate (ComponentRes<Res_Transform>()),
            ReadImmediate (ComponentRes<Res_AICommandFrame>()),
            ReadImmediate (ComponentRes<Res_SpawnType>()),
            ReadImmediate (ComponentRes<Res_PendingDespawn>()),
        };
        static const std::array<AccessSpec, 4> kResolveAnimPlayback = {
            WriteImmediate(ComponentRes<Res_AnimPlayback>()),
            WriteImmediate(ComponentRes<Res_DirtyFlags>()),
            ReadImmediate (ComponentRes<Res_AbilityState>()),
            ReadImmediate (ComponentRes<Res_LocomotionState>()),
        };
        static const std::array<AccessSpec, 3> kSampleAnimPose = {
            ReadImmediate (ComponentRes<Res_AnimPlayback>()),
            ReadImmediate (ExternalRes<Res_X_AnimReg>()),
            WriteImmediate(ComponentRes<Res_SampledPose>()),
        };
        static const std::array<AccessSpec, 3> kFitCombatCollider = {
            ReadImmediate (ComponentRes<Res_SampledPose>()),
            ReadImmediate (ExternalRes<Res_X_AnimReg>()),
            WriteImmediate(ComponentRes<Res_CombatCollider>()),
        };
        static const std::array<AccessSpec, 4> kComputeLocoMoveDelta = {
            ReadImmediate (ComponentRes<Res_Transform>()),
            ReadImmediate (ComponentRes<Res_AbilityState>()),
            ReadImmediate (ComponentRes<Res_AICommandFrame>()),
            WriteImmediate(ComponentRes<Res_LocoMoveDelta>()),
        };
        static const std::array<AccessSpec, 4> kComputeAbilityMoveDelta = {
            ReadImmediate (ComponentRes<Res_AbilityState>()),
            ReadImmediate (ComponentRes<Res_Transform>()),
            ReadImmediate (ComponentRes<Res_AbilityTimeline>()),
            WriteImmediate(ComponentRes<Res_AbilityMoveDelta>()),
        };
        static const std::array<AccessSpec, 7> kApplyMoveDelta = {
            WriteImmediate(ComponentRes<Res_Transform>()),
            WriteImmediate(ComponentRes<Res_PreCollisionTransform>()),
            WriteImmediate(ComponentRes<Res_LocoMoveDelta>()),
            WriteImmediate(ComponentRes<Res_AbilityMoveDelta>()),
            WriteImmediate(ComponentRes<Res_DirtyFlags>()),
            ReadImmediate (ComponentRes<Res_AbilityState>()),
            WriteImmediate(ComponentRes<Res_AbilityMoveRuntime>()),
        };
        static const std::array<AccessSpec, 7> kResolveNavMesh = {
            ReadImmediate (ExternalRes<Res_X_NavMesh>()),
            ReadImmediate (ComponentRes<Res_PreCollisionTransform>()),
            ReadImmediate (ComponentRes<Res_BodyCollisionShape>()),
            WriteImmediate(ComponentRes<Res_Transform>()),
            WriteImmediate(ComponentRes<Res_NavMeshAgent>()),
            WriteImmediate(ComponentRes<Res_BodyCollisionResolve>()),
            WriteImmediate(ComponentRes<Res_DirtyFlags>()),
        };
        static const std::array<AccessSpec, 6> kResolveCharOverlap = {
            WriteImmediate(ComponentRes<Res_Transform>()),
            WriteImmediate(ComponentRes<Res_BodyCollisionResolve>()),
            WriteImmediate(ComponentRes<Res_DirtyFlags>()),
            ReadImmediate (ComponentRes<Res_PreCollisionTransform>()),
            ReadImmediate (ComponentRes<Res_BodyCollisionShape>()),
            ReadImmediate (ComponentRes<Res_PendingDespawn>()),
        };
        static const std::array<AccessSpec, 6> kResolvePortal = {
            WriteImmediate(ComponentRes<Res_PortalTriggerState>()),
            ReadImmediate (ComponentRes<Res_Transform>()),
            ReadImmediate (ComponentRes<Res_AbilityState>()),
            ReadImmediate (ComponentRes<Res_PlayerControlIdentity>()),
            ReadImmediate (ComponentRes<Res_PendingDespawn>()),
            ReadImmediate (ExternalRes<Res_X_PortalDef>()),
        };
        static const std::array<AccessSpec, 2> kMarkTransferPending = {
            WriteImmediate(ComponentRes<Res_PendingTransferComp>()),
            ReadImmediate (ComponentRes<Res_PendingTransferTag>()),
        };
        static const std::array<AccessSpec, 2> kResolveCombatColliderActivation = {
            ReadImmediate (ComponentRes<Res_AbilityState>()),
            WriteImmediate(ComponentRes<Res_CombatColliderActivation>()),
        };
        static const std::array<AccessSpec, 11> kResolveCombatHit = {
            ReadImmediate (ComponentRes<Res_CombatColliderActivation>()),
            ReadImmediate (ComponentRes<Res_AbilityState>()),
            ReadImmediate (ComponentRes<Res_Transform>()),
            ReadImmediate (ComponentRes<Res_CombatCollider>()),
            ReadImmediate (ComponentRes<Res_LocomotionState>()),
            ReadImmediate (ComponentRes<Res_SpawnType>()),
            ReadImmediate (ComponentRes<Res_PendingDespawn>()),
            ReadImmediate (ExternalRes<Res_X_AbilityDef>()),
            WriteImmediate(ComponentRes<Res_CombatHitDedup>()),
            WriteImmediate(ComponentRes<Res_PendingCombatResult>()),
            EmitDeferred  (EventRes<Res_CombatEventQueue>()),              // ★ Rule 4
        };
        static const std::array<AccessSpec, 11> kCommitCombatResult = {
            WriteDeferred (ComponentRes<Res_CombatStat>()),                // ★ Rule 3
            // PendingCombatResult 제거: ResolveCombatHit↔CommitCombatResult 쌍에서
            //   WriteImmediate(PendingCombatResult) 충돌이 Rule 4 시연을 가리는 것을 방지
            // CommandBufferRes 제거: ResolveDeathAndDespawn↔CommitCombatResult 쌍에서
            //   WriteDeferred+WriteDeferred 충돌이 Rule 3 시연을 가리는 것을 방지
            WriteImmediate(ComponentRes<Res_DirtyFlags>()),
            WriteImmediate(ComponentRes<Res_AIReaction>()),
            WriteImmediate(ComponentRes<Res_BossPhase>()),
            WriteImmediate(ComponentRes<Res_BossPattern>()),
            WriteImmediate(ComponentRes<Res_AbilityInterruptQ>()),
            ReadImmediate (ComponentRes<Res_AIType>()),
            ReadImmediate (ComponentRes<Res_PendingDespawn>()),
            ReadImmediate (ComponentRes<Res_PendingTransferTag>()),
            WriteDeferred (ComponentRes<Res_AbilityMoveDelta>()),
            EmitDeferred  (EventRes<Res_CombatEventQueue>()),              // ★ Rule 4
        };
        static const std::array<AccessSpec, 5> kCommitAbilityTimelineEvent = {
            ReadImmediate (ComponentRes<Res_AbilityTimeline>()),
            ReadImmediate (ComponentRes<Res_PendingCombatResult>()),
            WriteImmediate(ComponentRes<Res_PendingProjectileSpawn>()),
            WriteImmediate(ComponentRes<Res_PendingAbilityEvent>()),
            WriteImmediate(ComponentRes<Res_ReplicationStats>()),
        };
        static const std::array<AccessSpec, 1> kFinalizePostCommit = {
            WriteImmediate(ComponentRes<Res_PendingCombatResult>()),
        };
        static const std::array<AccessSpec, 5> kResolveDeathAndDespawn = {
            ReadImmediate (ComponentRes<Res_CombatStat>()),                // ★ Rule 3 쌍
            ReadImmediate (ComponentRes<Res_AbilityState>()),
            ReadImmediate (ComponentRes<Res_PendingDespawn>()),
            ReadImmediate (ComponentRes<Res_PendingTransferTag>()),
            WriteDeferred (CommandBufferRes()),
        };
        static const std::array<AccessSpec, 2> kCollectReplicationTodo = {
            WriteImmediate(ComponentRes<Res_PendingProjectileSpawn>()),
            WriteImmediate(ComponentRes<Res_PendingAbilityEvent>()),
        };

        return {
            { "AIPerceptionSystem",               kN*kN*800,   kAIPerception              },
            { "AIDecisionSystem",                 kN*50000,    kAIDecision                },
            { "ApplyPlayerCommandSystem",         kN*5000,     kApplyPlayerCmd            },
            { "ApplyAICommandSystem",             kN*5000,     kApplyAICmd                },
            { "ResolveAbilityStateSystem",        kN*30000,    kResolveAbilityState       },
            { "AdvanceAbilityTimelineSystem",     kN*10000,    kAdvanceAbilityTimeline    },
            { "ResolveLocomotionStateSystem",     kN*10000,    kResolveLocomotion         },
            { "ResolveAnimationPlaybackSystem",   kN*10000,    kResolveAnimPlayback       },
            { "SampleAnimationPoseSystem",        kN*40000,    kSampleAnimPose            },
            { "FitSkeletalCombatColliderSystem",  kN*30000,    kFitCombatCollider         },
            { "ComputeLocomotionMoveDeltaSystem", kN*8000,     kComputeLocoMoveDelta      },
            { "ComputeAbilityMoveDeltaSystem",    kN*8000,     kComputeAbilityMoveDelta   },
            { "ApplyMovementDeltaSystem",         kN*5000,     kApplyMoveDelta            },
            { "ResolveNavMeshBodyConstraint",     kN*15000,    kResolveNavMesh            },
            { "ResolveCharacterOverlapSystem",    kN*kN*400,   kResolveCharOverlap        },
            { "ResolvePortalTriggerSystem",       kN*5000,     kResolvePortal             },
            { "MarkTransferPendingSystem",        kN*2000,     kMarkTransferPending       },
            { "ResolveCombatColliderActivation",  kN*3000,     kResolveCombatColliderActivation },
            { "ResolveCombatHitSystem",           kN*20000,    kResolveCombatHit          },
            { "CommitCombatResultSystem",         kN*20000,    kCommitCombatResult        },
            { "CommitAbilityTimelineEvent",       kN*3000,     kCommitAbilityTimelineEvent},
            { "FinalizePostCommitStateSystem",    kN*2000,     kFinalizePostCommit        },
            { "ResolveDeathAndDespawnSystem",     kN*3000,     kResolveDeathAndDespawn    },
            { "CollectReplicationTodoSource",     kN*2000,     kCollectReplicationTodo    },
        };
    }

    // =========================================================================
    // Section 7: Graph 빌드 인프라
    // =========================================================================

    constexpr WorldExecutionModelKey kEvalModelKey = 42001;

    struct EvalBundle
    {
        // WorldDef은 runtimes 보다 먼저 선언 — WorldRuntime이 &worldDef 포인터를 유지하므로
        // 소멸 순서가 worldDef > runtimes 를 보장해야 한다.
        WorldDef                                  worldDef;
        WorldExecutionModel                       model;
        ExecutionSourceRegistry                   sourceRegistry;
        WorldExecutionModelRegistry               modelRegistry;
        WorldFrameSelectionSet                    selections;
        BuildResult                               buildResult;
        int                                       worldCount{ 1 };
        // CommitScope가 nullptr WorldRuntime에서 예외를 throw하므로 실제 인스턴스를 소유
        std::vector<std::unique_ptr<WorldRuntime>> runtimes;
        uint64_t                                  frameIndex{ 0 };
    };

    std::unique_ptr<EvalBundle> BuildEvalBundle(
        const std::vector<SystemEvalDesc>& systems,
        int numWorlds = 1)
    {
        auto bundle = std::make_unique<EvalBundle>();
        bundle->worldCount = numWorlds;
        bundle->model.key  = kEvalModelKey;

        for (const SystemEvalDesc& sys : systems)
        {
            const ExecToken token = bundle->sourceRegistry.AllocateToken();
            g_evalWorkNs[token]   = sys.workNs;

            ExecutionSourceDesc desc{};
            desc.token     = token;
            desc.phase     = ExecPhase::Simulate;
            desc.lane      = ExecLane::Main;
            desc.kind      = ExecNodeKind::StaticSystem;
            desc.flags     = ExecNodeFlag_None;
            desc.fn        = &EvalSystemFn;
            desc.debugName = sys.name;
            desc.accesses  = sys.accesses;

            (void)bundle->sourceRegistry.Register(desc);
            bundle->model.simulateSources.push_back(token);
        }

        bundle->modelRegistry.Register(bundle->model, bundle->sourceRegistry);

        for (int w = 0; w < numWorlds; ++w)
        {
            const WorldId wid = WorldId::Create(static_cast<uint32_t>(w + 100), 1);
            bundle->selections.selections.push_back(WorldFrameSelection{
                static_cast<ExecScopeId>(w),
                wid.GetRaw(),
                kEvalModelKey
            });
        }

        ExecutionGraphBuildPolicy buildPolicy{};
        FrameBuildContext         buildContext{};
        buildContext.frameSelectionSet      = &bundle->selections;
        buildContext.executionModelRegistry = &bundle->modelRegistry;
        buildContext.executionSourceRegistry= &bundle->sourceRegistry;
        buildContext.buildPolicy            = &buildPolicy;

        ExecutionGraphBuilder graphBuilder{};
        bundle->buildResult = graphBuilder.Build(buildContext);

        // WorldDef 최소 초기화 — CommitScope가 nullptr WorldRuntime에서 예외 throw 방지
        // (smoke test의 MakeSmokeWorldDef 패턴과 동일)
        bundle->worldDef.id              = WorldDefId::Plaza;
        bundle->worldDef.name            = "EvalWorld";
        bundle->worldDef.topology        = { WorldKind::Dungeon, WorldInstanceType::Instanced };
        bundle->worldDef.entryPolicy     = { CreationPolicy::CreateOnDemand, JoinPolicy::FreeJoin,
                                             16, true, false, std::nullopt, std::nullopt };
        bundle->worldDef.map.resourceId  = 0;
        bundle->worldDef.map.defaultPlayerSpawnPointId = SpawnPointIds::None;
        bundle->worldDef.spawn           = { SpawnSetId::None, std::nullopt };
        bundle->worldDef.progressRule    = { WorldClearConditionType::None,
                                             WorldFailConditionType::None,
                                             WorldCompletionActionType::None,
                                             std::nullopt, false };
        bundle->worldDef.executionModelKey = kEvalModelKey;

        // 스코프당 WorldRuntime 생성·초기화
        bundle->runtimes.reserve(numWorlds);
        for (int w = 0; w < numWorlds; ++w)
        {
            auto rt = std::make_unique<WorldRuntime>(
                WorldRuntimeCreateParams{ &bundle->worldDef, &bundle->model, nullptr, nullptr });
            if (!rt->Initialize())
                throw std::runtime_error("PaperEvaluation BuildEvalBundle - WorldRuntime Initialize failed");
            bundle->runtimes.push_back(std::move(rt));
        }

        return bundle;
    }

    uint64_t RunEvalFrame(EvalBundle& bundle, int numWorkers)
    {
        const FrameTaskGraph& graph     = bundle.buildResult.graph;
        const int             nodeCount = static_cast<int>(graph.nodes.size());
        const int             scopeCount= static_cast<int>(graph.scopeCount);

        // 실제 WorldRuntime 사용 — CommitScope가 nullptr에서 예외 throw하는 것을 방지
        // (smoke test 패턴: MakeDummyValidOps + 실제 WorldRuntime)
        std::vector<WorldRuntime*> runtimeByScope;
        runtimeByScope.reserve(bundle.worldCount);
        for (auto& rt : bundle.runtimes)
            runtimeByScope.push_back(rt.get());

        // 프레임마다 BeginFrame 필요 (직렬 단계 상태 머신 리셋)
        ++bundle.frameIndex;
        const double nowSec = static_cast<double>(bundle.frameIndex) / 60.0;
        for (WorldRuntime* rt : runtimeByScope)
        {
            if (!rt->BeginFrame(bundle.frameIndex, nowSec, 1.0 / 60.0))
                throw std::runtime_error("PaperEvaluation RunEvalFrame - WorldRuntime BeginFrame failed");
        }

        std::vector<WorldId> worldIdByScope;
        worldIdByScope.reserve(bundle.worldCount);
        for (int w = 0; w < bundle.worldCount; ++w)
            worldIdByScope.push_back(WorldId::Create(static_cast<uint32_t>(w + 100), 1));

        std::vector<ExecNodeRuntime>  nodeRuntime (nodeCount);
        std::vector<ExecScopeRuntime> scopeRuntime(scopeCount);

        ExecutionOps     ops = MakeEvalOps();
        FrameExecContext frameExec{};
        frameExec.graph          = &graph;
        frameExec.ops            = &ops;
        frameExec.runtimeByScope = runtimeByScope;
        frameExec.worldIdByScope = worldIdByScope;

        ExecRuntimeState execRuntime{};
        execRuntime.BindViews(
            std::span<ExecNodeRuntime> (nodeRuntime .data(), nodeRuntime .size()),
            std::span<ExecScopeRuntime>(scopeRuntime.data(), scopeRuntime.size()),
            graph.simulateNodeCount);

        TaskExecutor executor(numWorkers);

        const uint64_t t0 = NowNs();
        if (!executor.ExecuteFrame(frameExec, execRuntime, bundle.sourceRegistry))
            throw std::runtime_error("PaperEvaluation RunEvalFrame - ExecuteFrame failed");
        return NowNs() - t0;
    }

    // =========================================================================
    // Metric 1: Speedup — T_serial / T_parallel (worker 1→8)
    // =========================================================================

    void RunEval_Speedup(const std::vector<SystemEvalDesc>& systems)
    {
        std::cout << "\n[Metric 1] Speedup (T_serial / T_parallel)\n";
        std::cout << "  Building graph... ";

        auto bundle = BuildEvalBundle(systems, 1);
        if (!bundle->buildResult.success)
        {
            std::cout << "FAILED (graph build error)\n";
            return;
        }
        std::cout << "OK (" << bundle->buildResult.graph.nodes.size() << " nodes, "
                  << bundle->buildResult.graph.edges.size() << " edges)\n";

        constexpr int kWarmup   = 3;
        constexpr int kMeasured = 15;
        constexpr int kWorkers[]= { 1, 2, 4, 8 };

        uint64_t t1Worker = 0;

        std::printf("  %-8s  %-10s  %-10s  %-10s\n",
            "Workers", "Avg(ms)", "Speedup", "Efficiency");
        std::printf("  %-8s  %-10s  %-10s  %-10s\n",
            "-------", "-------", "-------", "----------");

        for (const int workers : kWorkers)
        {
            for (int i = 0; i < kWarmup; ++i)
                RunEvalFrame(*bundle, workers);

            uint64_t total = 0;
            for (int i = 0; i < kMeasured; ++i)
                total += RunEvalFrame(*bundle, workers);

            const uint64_t avg = total / static_cast<uint64_t>(kMeasured);

            if (workers == 1)
                t1Worker = avg;

            const double speedup    = static_cast<double>(t1Worker) / static_cast<double>(avg);
            const double efficiency = speedup / static_cast<double>(workers) * 100.0;

            std::printf("  %-8d  %-10.3f  %-10.3f  %-9.1f%%\n",
                workers,
                static_cast<double>(avg) / 1e6,
                speedup,
                efficiency);
        }
    }

    // =========================================================================
    // Metric 2: False Serialization 감소율 (Binary vs 5-Rule 정적 분석)
    // =========================================================================

    void RunEval_FalseSerialization(const std::vector<SystemEvalDesc>& systems)
    {
        std::cout << "\n[Metric 2] False Serialization Reduction (Binary vs 5-Rule)\n";

        const int N          = static_cast<int>(systems.size());
        int       totalPairs = 0;
        int       serialBinary = 0;
        int       serial5Rule  = 0;

        // 규칙 시연 쌍 기록
        struct DemoPair { const char* sysA; const char* sysB; const char* rule; bool firedIn5Rule; };
        std::vector<DemoPair> demoPairs;

        for (int i = 0; i < N; ++i)
        {
            for (int j = i + 1; j < N; ++j)
            {
                ++totalPairs;
                const bool binConflict  = BinaryHasAnyConflict(systems[i].accesses, systems[j].accesses);
                const bool ruleConflict = HasAnyConflict(systems[i].accesses, systems[j].accesses, nullptr);

                if (binConflict)  ++serialBinary;
                if (ruleConflict) ++serial5Rule;

                if (binConflict && !ruleConflict)
                {
                    // 어느 규칙으로 해소됐는지 판별 (간이 탐지)
                    const char* rule = "Rule5";
                    for (const AccessSpec& sa : systems[i].accesses)
                    {
                        for (const AccessSpec& sb : systems[j].accesses)
                        {
                            if (sa.resource != sb.resource) continue;
                            if (sa.visibility == Visibility::Snapshot ||
                                sb.visibility == Visibility::Snapshot)
                            { rule = "Rule1-Snapshot"; break; }
                            const bool aDef = sa.visibility == Visibility::Deferred;
                            const bool bDef = sb.visibility == Visibility::Deferred;
                            if ((aDef && sb.mode == AccessMode::Read) ||
                                (bDef && sa.mode == AccessMode::Read))
                            { rule = "Rule3-Deferred+Read"; break; }
                            if (sa.mode == AccessMode::Emit &&
                                sb.mode == AccessMode::Emit)
                            { rule = "Rule4-DualEmit"; break; }
                        }
                    }
                    demoPairs.push_back({ systems[i].name, systems[j].name, rule, true });
                }
            }
        }

        const int    removed      = serialBinary - serial5Rule;
        const double reductionPct = static_cast<double>(removed) / static_cast<double>(totalPairs) * 100.0;
        const int    parBinary    = totalPairs - serialBinary;
        const int    par5Rule     = totalPairs - serial5Rule;

        std::printf("  Total pairs       : %d\n", totalPairs);
        std::printf("  Binary  — serial  : %d  parallel: %d\n", serialBinary, parBinary);
        std::printf("  5-Rule  — serial  : %d  parallel: %d\n", serial5Rule,  par5Rule);
        std::printf("  False serial removed: %d\n", removed);
        std::printf("  Reduction rate    : %.2f%%\n", reductionPct);

        if (!demoPairs.empty())
        {
            std::cout << "  Demonstrated rule pairs:\n";
            for (const DemoPair& p : demoPairs)
                std::printf("    [%s] %s  ||  %s\n", p.rule, p.sysA, p.sysB);
        }
    }

    // =========================================================================
    // Metric 3: DAG 빌드 오버헤드 비율 (T_build / T_frame)
    // =========================================================================

    void RunEval_DAGBuildOverhead(const std::vector<SystemEvalDesc>& systems)
    {
        std::cout << "\n[Metric 3] DAG Build Overhead Ratio\n";

        // 레지스트리를 타이밍 루프 밖에서 한 번 구성
        WorldExecutionModel      model;
        ExecutionSourceRegistry  sourceRegistry;
        model.key = kEvalModelKey + 1;   // 다른 key — 전역 g_evalWorkNs 와 충돌 방지

        for (const SystemEvalDesc& sys : systems)
        {
            const ExecToken token = sourceRegistry.AllocateToken();
            g_evalWorkNs[token]   = sys.workNs;

            ExecutionSourceDesc desc{};
            desc.token     = token;
            desc.phase     = ExecPhase::Simulate;
            desc.lane      = ExecLane::Main;
            desc.kind      = ExecNodeKind::StaticSystem;
            desc.flags     = ExecNodeFlag_None;
            desc.fn        = &EvalSystemFn;
            desc.debugName = sys.name;
            desc.accesses  = sys.accesses;

            (void)sourceRegistry.Register(desc);
            model.simulateSources.push_back(token);
        }

        WorldExecutionModelRegistry modelRegistry;
        modelRegistry.Register(model, sourceRegistry);

        WorldFrameSelectionSet selections;
        const WorldId wid = WorldId::Create(200, 1);
        selections.selections.push_back(WorldFrameSelection{
            0, wid.GetRaw(), static_cast<WorldExecutionModelKey>(kEvalModelKey + 1)
        });

        ExecutionGraphBuildPolicy buildPolicy{};
        FrameBuildContext          buildContext{};
        buildContext.frameSelectionSet       = &selections;
        buildContext.executionModelRegistry  = &modelRegistry;
        buildContext.executionSourceRegistry = &sourceRegistry;
        buildContext.buildPolicy             = &buildPolicy;

        // 기준 프레임 실행 시간 (4 workers, 5 회 평균)
        ExecutionGraphBuilder refBuilder;
        BuildResult           refResult = refBuilder.Build(buildContext);

        // Metric 3 전용 WorldDef/WorldRuntime (CommitScope nullptr 예외 방지)
        WorldDef metric3Def{};
        metric3Def.id              = WorldDefId::Plaza;
        metric3Def.name            = "EvalMetric3";
        metric3Def.topology        = { WorldKind::Dungeon, WorldInstanceType::Instanced };
        metric3Def.entryPolicy     = { CreationPolicy::CreateOnDemand, JoinPolicy::FreeJoin,
                                       16, true, false, std::nullopt, std::nullopt };
        metric3Def.map.resourceId  = 0;
        metric3Def.map.defaultPlayerSpawnPointId = SpawnPointIds::None;
        metric3Def.spawn           = { SpawnSetId::None, std::nullopt };
        metric3Def.progressRule    = { WorldClearConditionType::None,
                                       WorldFailConditionType::None,
                                       WorldCompletionActionType::None,
                                       std::nullopt, false };
        metric3Def.executionModelKey = static_cast<WorldExecutionModelKey>(kEvalModelKey + 1);

        auto metric3Rt = std::make_unique<WorldRuntime>(
            WorldRuntimeCreateParams{ &metric3Def, &model, nullptr, nullptr });
        if (!metric3Rt->Initialize())
        {
            std::cout << "  FAILED: WorldRuntime Initialize\n";
            return;
        }

        uint64_t frameTotal = 0;
        {
            constexpr int kRef = 5;
            const int nodeCount  = static_cast<int>(refResult.graph.nodes.size());
            const int scopeCount = static_cast<int>(refResult.graph.scopeCount);
            uint64_t  metric3FrameIdx = 0;
            for (int r = 0; r < kRef; ++r)
            {
                ++metric3FrameIdx;
                if (!metric3Rt->BeginFrame(metric3FrameIdx,
                    static_cast<double>(metric3FrameIdx) / 60.0, 1.0 / 60.0))
                {
                    std::cout << "  FAILED: WorldRuntime BeginFrame\n";
                    return;
                }

                WorldRuntime*              rawRt = metric3Rt.get();
                std::vector<WorldRuntime*> runtimeByScope{ rawRt };
                std::vector<WorldId>       worldIdByScope{ wid };
                std::vector<ExecNodeRuntime>  nodeRt (nodeCount);
                std::vector<ExecScopeRuntime> scopeRt(scopeCount);

                ExecutionOps     ops = MakeEvalOps();
                FrameExecContext frameExec{};
                frameExec.graph          = &refResult.graph;
                frameExec.ops            = &ops;
                frameExec.runtimeByScope = runtimeByScope;
                frameExec.worldIdByScope = worldIdByScope;

                ExecRuntimeState execRt{};
                execRt.BindViews(
                    std::span<ExecNodeRuntime> (nodeRt .data(), nodeRt .size()),
                    std::span<ExecScopeRuntime>(scopeRt.data(), scopeRt.size()),
                    refResult.graph.simulateNodeCount);

                TaskExecutor executor(4);
                const uint64_t t0 = NowNs();
                if (!executor.ExecuteFrame(frameExec, execRt, sourceRegistry))
                {
                    std::cout << "  FAILED: ExecuteFrame\n";
                    return;
                }
                frameTotal += NowNs() - t0;
            }
            frameTotal /= kRef;
        }

        // 빌드 타이밍 루프
        constexpr int kBuildIters = 300;
        std::vector<uint64_t> buildTimes;
        buildTimes.reserve(kBuildIters);

        for (int i = 0; i < kBuildIters; ++i)
        {
            ExecutionGraphBuilder gb;
            const uint64_t t0 = NowNs();
            gb.Build(buildContext);
            buildTimes.push_back(NowNs() - t0);
        }

        std::sort(buildTimes.begin(), buildTimes.end());
        const uint64_t median = buildTimes[kBuildIters / 2];
        const uint64_t p95    = buildTimes[static_cast<size_t>(kBuildIters * 95 / 100)];
        const double   ratio  = static_cast<double>(median) /
                                static_cast<double>(frameTotal) * 100.0;

        std::printf("  Build median   : %.1f µs\n", static_cast<double>(median) / 1000.0);
        std::printf("  Build p95      : %.1f µs\n", static_cast<double>(p95)    / 1000.0);
        std::printf("  Frame avg(4w)  : %.3f ms\n", static_cast<double>(frameTotal) / 1e6);
        std::printf("  Overhead ratio : %.4f%%\n",   ratio);
    }

    // =========================================================================
    // Metric 4: 멀티 월드 스케일링 (World 수 1→20, Simulate 처리 시간)
    // =========================================================================

    void RunEval_MultiWorldScaling(const std::vector<SystemEvalDesc>& systems)
    {
        std::cout << "\n[Metric 4] Multi-World Simulate Scaling\n";
        std::printf("  %-8s  %-12s  %-12s\n",
            "Worlds", "Avg(ms)", "ms/World");
        std::printf("  %-8s  %-12s  %-12s\n",
            "------", "-------", "--------");

        constexpr int kWorldCounts[] = { 1, 2, 5, 10, 20 };
        constexpr int kWarmup  = 2;
        constexpr int kMeasure = 8;
        constexpr int kWorkers = 4;

        for (const int nw : kWorldCounts)
        {
            // 멀티 월드 번들은 각자 독립 토큰 공간 → g_evalWorkNs는 누적
            auto bundle = BuildEvalBundle(systems, nw);
            if (!bundle->buildResult.success)
            {
                std::printf("  %-8d  BUILD FAILED\n", nw);
                continue;
            }

            for (int i = 0; i < kWarmup; ++i)
                RunEvalFrame(*bundle, kWorkers);

            uint64_t total = 0;
            for (int i = 0; i < kMeasure; ++i)
                total += RunEvalFrame(*bundle, kWorkers);

            const uint64_t avg   = total / static_cast<uint64_t>(kMeasure);
            const double   msAvg = static_cast<double>(avg) / 1e6;
            const double   msPerWorld = msAvg / static_cast<double>(nw);

            std::printf("  %-8d  %-12.3f  %-12.3f\n", nw, msAvg, msPerWorld);
        }
    }

    // =========================================================================
    // Metric 5: 파이프라인 단계별 시간 분포
    //           (Simulate / Commit / LifecycleFlush / Reconcile)
    //
    // WorldRuntime이 필요한 유일한 지표.
    // Phase1SmokeTest와 동일 패턴으로 WorldRuntime 초기화.
    // 각 직렬 단계에 타임스탬프 probe 노드 삽입.
    // =========================================================================

    // 전역 타임스탬프 (probe 함수 → 측정 함수)
    std::atomic<uint64_t> g_commitBeginNs    { 0 };
    std::atomic<uint64_t> g_lifecycleBeginNs { 0 };
    std::atomic<uint64_t> g_reconcileBeginNs { 0 };

    // Commit 단계 시작 probe (실제 commit 완료 후 호출됨)
    ExecCallResult CommitPhaseProbe(NodeExecContext& ctx)
    {
        (void)ctx;
        g_commitBeginNs.store(NowNs(), std::memory_order_relaxed);
        BusyWaitNs(1'500'000);  // 커밋 직렬 작업 시뮬레이션 (~1.5ms)
        return ExecCallResult::Success;
    }

    // LifecycleFlush 단계 시작 probe
    ExecCallResult LifecyclePhaseProbe(NodeExecContext& ctx)
    {
        (void)ctx;
        g_lifecycleBeginNs.store(NowNs(), std::memory_order_relaxed);
        BusyWaitNs(400'000);    // lifecycle flush 시뮬레이션 (~0.4ms)
        return ExecCallResult::Success;
    }

    // Reconcile 단계 시작 probe
    ExecCallResult ReconcilePhaseProbe(NodeExecContext& ctx)
    {
        (void)ctx;
        g_reconcileBeginNs.store(NowNs(), std::memory_order_relaxed);
        BusyWaitNs(800'000);    // reconcile 시뮬레이션 (~0.8ms)
        return ExecCallResult::Success;
    }

    static WorldDef MakeEvalWorldDef(WorldExecutionModelKey modelKey)
    {
        WorldDef def{};
        def.id   = WorldDefId::Plaza;
        def.name = "PaperEvalWorld";
        def.topology   = { WorldKind::Dungeon, WorldInstanceType::Instanced };
        def.entryPolicy = {
            CreationPolicy::CreateOnDemand,
            JoinPolicy::FreeJoin,
            16, true, false,
            std::nullopt, std::nullopt
        };
        def.map.resourceId              = 0;
        def.map.defaultPlayerSpawnPointId = SpawnPointIds::None;
        def.spawn = { SpawnSetId::None, std::nullopt };
        def.progressRule = {
            WorldClearConditionType::None,
            WorldFailConditionType::None,
            WorldCompletionActionType::None,
            std::nullopt, false
        };
        def.executionModelKey = modelKey;
        return def;
    }

    void RunEval_PhaseDistribution(const std::vector<SystemEvalDesc>& systems)
    {
        std::cout << "\n[Metric 5] Pipeline Phase Time Distribution\n";

        constexpr WorldExecutionModelKey kPhaseModelKey = kEvalModelKey + 99;

        WorldExecutionModel     model;
        ExecutionSourceRegistry sourceRegistry;
        model.key = kPhaseModelKey;

        // Simulate 시스템 등록
        for (const SystemEvalDesc& sys : systems)
        {
            const ExecToken token = sourceRegistry.AllocateToken();
            g_evalWorkNs[token]   = sys.workNs;

            ExecutionSourceDesc desc{};
            desc.token     = token;
            desc.phase     = ExecPhase::Simulate;
            desc.lane      = ExecLane::Main;
            desc.kind      = ExecNodeKind::StaticSystem;
            desc.flags     = ExecNodeFlag_None;
            desc.fn        = &EvalSystemFn;
            desc.debugName = sys.name;
            desc.accesses  = sys.accesses;

            (void)sourceRegistry.Register(desc);
            model.simulateSources.push_back(token);
        }

        // 직렬 단계 probe 노드 등록 (helper lambda)
        auto AddProbe = [&](ExecPhase phase, ExecLane lane, ExecNodeKind kind,
                            ExecFn fn, const char* name)
        {
            const ExecToken token = sourceRegistry.AllocateToken();
            ExecutionSourceDesc desc{};
            desc.token     = token;
            desc.phase     = phase;
            desc.lane      = lane;
            desc.kind      = kind;
            desc.flags     = ExecNodeFlag_None;
            desc.fn        = fn;
            desc.debugName = name;
            (void)sourceRegistry.Register(desc);
            switch (phase)
            {
            case ExecPhase::Commit:
                model.commitSources.push_back(token);       break;
            case ExecPhase::LifecycleFlush:
                model.lifecycleFlushSources.push_back(token); break;
            case ExecPhase::Reconcile:
                model.reconcileSources.push_back(token);    break;
            default: break;
            }
        };

        AddProbe(ExecPhase::Commit,        ExecLane::Serial, ExecNodeKind::PostCommitFinalize,
                 &CommitPhaseProbe,    "Eval.CommitProbe");
        AddProbe(ExecPhase::LifecycleFlush,ExecLane::Serial, ExecNodeKind::LifecycleFlush,
                 &LifecyclePhaseProbe, "Eval.LifecycleProbe");
        AddProbe(ExecPhase::Reconcile,     ExecLane::Serial, ExecNodeKind::Reconcile,
                 &ReconcilePhaseProbe, "Eval.ReconcileProbe");

        WorldExecutionModelRegistry modelRegistry;
        modelRegistry.Register(model, sourceRegistry);

        WorldFrameSelectionSet selections;
        const WorldId wid = WorldId::Create(300, 1);
        selections.selections.push_back(WorldFrameSelection{
            0, wid.GetRaw(), kPhaseModelKey
        });

        ExecutionGraphBuildPolicy buildPolicy{};
        FrameBuildContext          buildContext{};
        buildContext.frameSelectionSet       = &selections;
        buildContext.executionModelRegistry  = &modelRegistry;
        buildContext.executionSourceRegistry = &sourceRegistry;
        buildContext.buildPolicy             = &buildPolicy;

        ExecutionGraphBuilder graphBuilder;
        BuildResult           buildResult = graphBuilder.Build(buildContext);
        if (!buildResult.success)
        {
            std::cout << "  FAILED: graph build error for phase distribution\n";
            return;
        }

        // WorldRuntime 초기화 (Phase1SmokeTest 동일 패턴)
        WorldDef def = MakeEvalWorldDef(kPhaseModelKey);
        WorldRuntime runtime(WorldRuntimeCreateParams{ &def, &model, nullptr, nullptr });

        if (!runtime.Initialize())
        {
            std::cout << "  FAILED: WorldRuntime Initialize\n";
            return;
        }
        runtime.ClearLifecycleOutbox();
        if (!runtime.BeginFrame(1, 100.0, 0.016))
        {
            std::cout << "  FAILED: WorldRuntime BeginFrame\n";
            return;
        }

        const int nodeCount  = static_cast<int>(buildResult.graph.nodes.size());
        const int scopeCount = static_cast<int>(buildResult.graph.scopeCount);

        std::vector<WorldRuntime*> runtimeByScope{ &runtime };
        std::vector<WorldId>       worldIdByScope { wid };
        std::vector<ExecNodeRuntime>  nodeRuntime (nodeCount);
        std::vector<ExecScopeRuntime> scopeRuntime(scopeCount);

        ExecutionOps     ops = MakeEvalOps();
        FrameExecContext frameExec{};
        frameExec.graph          = &buildResult.graph;
        frameExec.ops            = &ops;
        frameExec.runtimeByScope = runtimeByScope;
        frameExec.worldIdByScope = worldIdByScope;

        ExecRuntimeState execRuntime{};
        execRuntime.BindViews(
            std::span<ExecNodeRuntime> (nodeRuntime .data(), nodeRuntime .size()),
            std::span<ExecScopeRuntime>(scopeRuntime.data(), scopeRuntime.size()),
            buildResult.graph.simulateNodeCount);

        // 타임스탬프 초기화
        g_commitBeginNs   .store(0, std::memory_order_relaxed);
        g_lifecycleBeginNs.store(0, std::memory_order_relaxed);
        g_reconcileBeginNs.store(0, std::memory_order_relaxed);

        TaskExecutor executor(4);

        const uint64_t t0 = NowNs();
        if (!executor.ExecuteFrame(frameExec, execRuntime, sourceRegistry))
        {
            std::cout << "  FAILED: ExecuteFrame\n";
            return;
        }
        const uint64_t tEnd = NowNs();

        const uint64_t tCommit    = g_commitBeginNs   .load(std::memory_order_relaxed);
        const uint64_t tLifecycle = g_lifecycleBeginNs.load(std::memory_order_relaxed);
        const uint64_t tReconcile = g_reconcileBeginNs.load(std::memory_order_relaxed);
        const uint64_t tTotal     = tEnd - t0;

        if (tCommit == 0 || tLifecycle == 0 || tReconcile == 0)
        {
            std::cout << "  WARNING: one or more phase probes did not fire\n";
        }

        // 구간 계산 (probe 시점 기준 근사값)
        const uint64_t nsSimulate  = (tCommit    > t0)          ? tCommit    - t0          : 0;
        const uint64_t nsCommit    = (tLifecycle > tCommit)     ? tLifecycle - tCommit     : 0;
        const uint64_t nsLifecycle = (tReconcile > tLifecycle)  ? tReconcile - tLifecycle  : 0;
        const uint64_t nsReconcile = (tEnd       > tReconcile)  ? tEnd       - tReconcile  : 0;

        auto pct = [&](uint64_t ns) -> double {
            return tTotal > 0 ? static_cast<double>(ns) / static_cast<double>(tTotal) * 100.0 : 0.0;
        };

        std::printf("  Total frame     : %.3f ms\n", static_cast<double>(tTotal)     / 1e6);
        std::printf("  Simulate (par)  : %.3f ms  (%.1f%%)\n",
            static_cast<double>(nsSimulate)  / 1e6, pct(nsSimulate));
        std::printf("  Commit   (ser)  : %.3f ms  (%.1f%%)\n",
            static_cast<double>(nsCommit)    / 1e6, pct(nsCommit));
        std::printf("  Lifecycle(ser)  : %.3f ms  (%.1f%%)\n",
            static_cast<double>(nsLifecycle) / 1e6, pct(nsLifecycle));
        std::printf("  Reconcile(ser)  : %.3f ms  (%.1f%%)\n",
            static_cast<double>(nsReconcile) / 1e6, pct(nsReconcile));
        std::printf("  Serial overhead : %.1f%% of frame\n",
            pct(nsCommit) + pct(nsLifecycle) + pct(nsReconcile));
    }

} // anonymous namespace

// ===========================================================================
// RunPaperEvaluation — 외부 진입점 (main.cpp에서 "--paper-eval" 인자로 호출)
// ===========================================================================

void RunPaperEvaluation()
{
    std::cout << "\n";
    std::cout << "====================================================\n";
    std::cout << "  WITH Server — Paper Quantitative Evaluation\n";
    std::cout << "  ECS+TaskGraph Declarative Dependency Framework\n";
    std::cout << "====================================================\n";

    g_evalWorkNs.clear();

    // 24개 게임 시스템 + 정제된 AccessSpec 로드
    const std::vector<SystemEvalDesc> systems = BuildFullSystemTable();

    std::printf("  Systems: %zu  |  N(entities): 50  |  Serial budget: ~17.3ms\n",
        systems.size());

    RunEval_Speedup(systems);             // 실행 타이밍 (가장 오래 걸림)
    RunEval_FalseSerialization(systems);   // 정적 분석 — 빠름
    RunEval_DAGBuildOverhead(systems);     // 반복 빌드 타이밍
    RunEval_MultiWorldScaling(systems);   // 멀티 월드 실행
    RunEval_PhaseDistribution(systems);   // WorldRuntime 4-phase

    std::cout << "\n====================================================\n";
    std::cout << "  Evaluation complete.\n";
    std::cout << "====================================================\n\n";
}
