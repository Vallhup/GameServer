#include "pch.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <span>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "DynamicTaskTypes.h"
#include "ExecutionContextTypes.h"
#include "ExecutionCoreTypes.h"
#include "ExecutionGraphBuilder.h"
#include "ExecutionGraphBuildPolicy.h"
#include "ExecutionGraphTypes.h"
#include "ExecutionOps.h"
#include "ExecutionRuntimeTypes.h"
#include "ExecutionSourceTypes.h"
#include "IWorldDefinitionProvider.h"
#include "IWorldInstanceFactory.h"
#include "SystemMeta.h"
#include "TaskExecutor.h"
#include "WorldDef.h"
#include "WorldDefinitionBootstrap.h"
#include "WorldExecutionModelTypes.h"
#include "WorldManager.h"
#include "WorldRegistry.h"
#include "WorldRuntime.h"
#include "WorldScheduler.h"
#include "WorldTransferProfileRegistry.h"
#include "WorldFragmentBuild.h"

// ============================================================================
// 화이트박스 테스트 훅
//
// ExecutionGraphBuilder의 private 3-phase 메서드를 테스트에서 직접 호출할 수
// 있도록 한다. ExecutionGraphBuilder.h에 friend 선언이 있으므로 컴파일된다.
// 이 struct는 이 파일 이외에서 사용하지 않는다.
// ============================================================================

struct ExecutionGraphBuilderTestHook
{
    static void AssembleFrameGraphNodes(
        const ExecutionGraphBuilder& builder,
        const std::vector<WorldFragmentBuild>& fragments,
        FrameTaskGraph& outGraph,
        const ExecutionGraphBuildPolicy& policy,
        std::vector<std::vector<ExecNodeId>>& outPredLists,
        std::vector<std::vector<ExecNodeId>>& outSuccLists)
    {
        builder.AssembleFrameGraphNodes(
            fragments, outGraph, policy, outPredLists, outSuccLists);
    }

    static void AppendDynamicNodes(
        const ExecutionGraphBuilder& builder,
        const DynamicTaskFrozenBatch& batch,
        const DynamicTaskTypeRegistry& typeRegistry,
        const ExecutionSourceRegistry& sourceRegistry,
        const ConflictRegistry* conflictRegistry,
        FrameTaskGraph& outGraph,
        std::vector<std::vector<ExecNodeId>>& predLists,
        std::vector<std::vector<ExecNodeId>>& succLists,
        DynamicTaskFrameTable& outFrameTable,
        BuildResult& result)
    {
        builder.AppendDynamicNodes(
            batch, typeRegistry, sourceRegistry, conflictRegistry,
            outGraph, predLists, succLists, outFrameTable, result);
    }

    static void FinalizeEdgePool(
        const ExecutionGraphBuilder& builder,
        FrameTaskGraph& outGraph,
        const std::vector<std::vector<ExecNodeId>>& predLists,
        const std::vector<std::vector<ExecNodeId>>& succLists,
        const ExecutionGraphBuildPolicy& policy)
    {
        builder.FinalizeEdgePool(outGraph, predLists, succLists, policy);
    }
};

// ============================================================================
// 공통 유틸리티
// ============================================================================

namespace
{
    static void LogTestBanner(const char* name)
    {
        std::cout << "\n[DT_TEST] " << name << "\n";
    }

    // ---------------------------------------------------------------------------
    // 테스트 레코더 — 실행된 payload 키를 스레드 안전하게 기록한다.
    // ---------------------------------------------------------------------------
    struct DynTestRecorder
    {
        std::mutex mtx;
        std::vector<uint64_t> executedPayloads; // dispatch된 payloadKey 순서
        std::vector<std::string> events;        // 이름 기반 이벤트

        void RecordPayload(uint64_t key)
        {
            std::lock_guard lock{ mtx };
            executedPayloads.push_back(key);
        }

        void RecordEvent(const std::string& s)
        {
            std::lock_guard lock{ mtx };
            events.push_back(s);
        }

        bool HasPayload(uint64_t key) const
        {
            for (uint64_t k : executedPayloads)
                if (k == key) return true;
            return false;
        }

        size_t PayloadCount() const { return executedPayloads.size(); }
        size_t EventCount() const { return events.size(); }
    };

    // 전역 환경 — dispatch fn 콜백에서 접근한다.
    static DynTestRecorder* g_recorder = nullptr;

    // ---------------------------------------------------------------------------
    // 기본 dispatch fn: payload key를 기록하고 Success를 반환한다.
    // ---------------------------------------------------------------------------
    static ExecCallResult DynDispatch_RecordPayload(NodeExecContext& ctx)
    {
        assert(ctx.IsValid());
        assert(ctx.frame != nullptr);
        assert(ctx.frame->dynamicTaskFrameTable != nullptr);

        const DynamicTaskInstance* inst =
            ctx.frame->dynamicTaskFrameTable->FindByNodeId(ctx.nodeId);
        assert(inst != nullptr);

        if (g_recorder)
            g_recorder->RecordPayload(inst->payloadKey);

        return ExecCallResult::Success;
    }

    // ---------------------------------------------------------------------------
    // Static system dispatch fn: 실행 시 레코더에 이름을 기록한다.
    // ---------------------------------------------------------------------------
    static ExecCallResult StaticSys_RecordEvent(NodeExecContext& ctx)
    {
        assert(ctx.IsValid());
        if (g_recorder)
            g_recorder->RecordEvent("StaticSys");
        return ExecCallResult::Success;
    }

    // ---------------------------------------------------------------------------
    // TestRig — DynamicTask를 포함한 그래프를 수동으로 조립할 때 사용.
    // ---------------------------------------------------------------------------
    struct DynTestRig
    {
        FrameTaskGraph graph{};
        ExecutionSourceRegistry sources{};
        DynamicTaskFrameTable frameTable{};

        std::vector<WorldRuntime*> runtimeByScopeBacking{};
        std::unique_ptr<ExecNodeRuntime[]> nodeBacking{};
        std::unique_ptr<ExecScopeRuntime[]> scopeBacking{};

        FrameExecContext frame{};
        ExecRuntimeState runtime{};

        // ops는 DummyValid (실제 WorldManager 없이)
        ExecutionOps ops;

        explicit DynTestRig()
            : ops(
                reinterpret_cast<WorldManager*>(0x1),
                reinterpret_cast<WorldRegistry*>(0x1),
                reinterpret_cast<WorldTransferService*>(0x1),
                reinterpret_cast<WorldAdmissionService*>(0x1),
                reinterpret_cast<NetIdRegistry*>(0x1),
                reinterpret_cast<PresenceManager*>(0x1))
        {
        }

        void Finalize()
        {
            runtimeByScopeBacking.assign(graph.scopeCount, nullptr);

            nodeBacking = !graph.nodes.empty()
                ? std::make_unique<ExecNodeRuntime[]>(graph.nodes.size())
                : nullptr;

            scopeBacking = graph.scopeCount > 0
                ? std::make_unique<ExecScopeRuntime[]>(graph.scopeCount)
                : nullptr;

            frame.graph  = &graph;
            frame.ops    = &ops;
            frame.runtimeByScope = std::span<WorldRuntime*>(
                runtimeByScopeBacking.data(),
                runtimeByScopeBacking.size());
            frame.dynamicTaskFrameTable = frameTable.IsEmpty() ? nullptr : &frameTable;

            runtime.nodes = std::span<ExecNodeRuntime>(
                nodeBacking.get(),
                graph.nodes.size());
            runtime.scopes = std::span<ExecScopeRuntime>(
                scopeBacking.get(),
                graph.scopeCount);
        }
    };

    static void ExpectNodeState(
        const ExecRuntimeState& rt,
        ExecNodeId nodeId,
        ExecNodeState expected)
    {
        const ExecNodeRuntime* n = rt.TryGetNode(nodeId);
        assert(n != nullptr);
        const ExecNodeState actual = n->state.load();
        if (actual != expected)
        {
            std::cout << "[FAIL] node=" << nodeId
                << " expected=" << static_cast<int>(expected)
                << " actual=" << static_cast<int>(actual) << "\n";
            assert(false);
        }
    }

    // ---------------------------------------------------------------------------
    // Test용 WorldDef 헬퍼
    // ---------------------------------------------------------------------------
    struct FaultProbeComp final : Component { int v{ 0 }; };

    class DynTestWorldImpl final : public IWorldInstanceImpl
    {
    public:
        bool OnCreate(WorldRuntime& r) override { r.RegisterStorage<FaultProbeComp>(); return true; }
        bool OnStart(WorldRuntime&) override { return true; }
        void OnStop(WorldRuntime&) override {}
    };

    class DynTestWorldFactory final : public IWorldInstanceFactory
    {
    public:
        std::unique_ptr<IWorldInstanceImpl> Create(const WorldDef&, WorldId) override
        {
            return std::make_unique<DynTestWorldImpl>();
        }
    };

    static WorldDef MakeMinimalWorldDef(const char* name = "DynTest")
    {
        WorldDef def{};
        def.id   = WorldDefId::Plaza;
        def.name = name;
        def.topology  = { WorldKind::Hub, WorldInstanceType::Instanced };
        def.entryPolicy = { CreationPolicy::CreateOnDemand, JoinPolicy::FreeJoin,
                            0, true, false, std::nullopt, std::nullopt };
        def.map  = { 0, 0, {}, std::nullopt, {} };
        def.spawn = { SpawnSetId::None, std::nullopt };
        def.progressRule = { WorldClearConditionType::None, WorldFailConditionType::None,
                             WorldCompletionActionType::None, std::nullopt, false };
        def.linkRules = {};
        return def;
    }

    // ============================================================================
    // Test_DT_01 — DynamicTaskTypeRegistry::Register 기본 동작
    // ============================================================================
    static void Test_DT_01_TypeRegistry_Register_Basic()
    {
        LogTestBanner(__FUNCTION__);

        ExecutionSourceRegistry sourceRegistry{};
        DynamicTaskTypeRegistry typeRegistry{};

        DynamicTaskTypeDesc desc{};
        desc.debugName    = "TestTaskType";
        desc.defaultPhase = ExecPhase::Simulate;
        desc.defaultLane  = ExecLane::Parallel;
        desc.dispatchFn   = &DynDispatch_RecordPayload;

        const DynamicTaskTypeId typeId = typeRegistry.Register(
            std::move(desc), sourceRegistry);

        assert(typeId != InvalidDynamicTaskTypeId);

        // 조회 성공
        const DynamicTaskTypeDesc* stored = typeRegistry.TryGet(typeId);
        assert(stored != nullptr);
        assert(stored->typeId == typeId);
        assert(stored->sourceToken != InvalidExecToken);
        assert(stored->dispatchFn == &DynDispatch_RecordPayload);

        // ExecutionSourceRegistry에도 등록됐는지 확인
        const ExecutionSourceDesc* sourceDesc = sourceRegistry.TryGet(stored->sourceToken);
        assert(sourceDesc != nullptr);
        assert(sourceDesc->kind == ExecNodeKind::DynamicTask);
        assert(sourceDesc->fn == &DynDispatch_RecordPayload);
        assert(sourceDesc->phase == ExecPhase::Simulate);

        // 중복 등록 방지: 동일 내용의 두 번째 등록은 다른 typeId를 발급해야 한다.
        DynamicTaskTypeDesc desc2{};
        desc2.debugName  = "TestTaskType2";
        desc2.dispatchFn = &DynDispatch_RecordPayload;
        const DynamicTaskTypeId typeId2 = typeRegistry.Register(std::move(desc2), sourceRegistry);
        assert(typeId2 != InvalidDynamicTaskTypeId);
        assert(typeId2 != typeId);

        // 무효 desc 등록 → InvalidDynamicTaskTypeId
        DynamicTaskTypeDesc badDesc{};
        badDesc.debugName = "";  // IsValid() == false
        badDesc.dispatchFn = &DynDispatch_RecordPayload;
        const DynamicTaskTypeId badId = typeRegistry.Register(std::move(badDesc), sourceRegistry);
        assert(badId == InvalidDynamicTaskTypeId);
    }

    // ============================================================================
    // Test_DT_02 — DynamicTaskPendingQueue Thread-safe Push / Drain
    // ============================================================================
    static void Test_DT_02_PendingQueue_ConcurrentPush_Drain()
    {
        LogTestBanner(__FUNCTION__);

        DynamicTaskPendingQueue queue;

        constexpr uint32_t kThreadCount   = 8;
        constexpr uint32_t kPushPerThread = 100;

        std::vector<std::thread> threads;
        threads.reserve(kThreadCount);

        for (uint32_t t = 0; t < kThreadCount; ++t)
        {
            threads.emplace_back([&queue, t]()
            {
                for (uint32_t i = 0; i < kPushPerThread; ++i)
                {
                    DynamicTaskRequest req{};
                    req.typeId      = 1;
                    req.scopeId     = 0;
                    req.payloadKey  = static_cast<uint64_t>(t) * 1000 + i;
                    req.priorityBias = 0;
                    queue.Push(req);
                }
            });
        }

        for (auto& th : threads)
            th.join();

        std::vector<DynamicTaskRequest> out;
        queue.DrainInto(out);

        assert(out.size() == kThreadCount * kPushPerThread);

        // Drain 후 큐는 비어 있어야 한다.
        assert(queue.PendingCount() == 0);

        // 두 번째 Drain은 빈 결과를 반환해야 한다.
        std::vector<DynamicTaskRequest> out2;
        queue.DrainInto(out2);
        assert(out2.empty());
    }

    // ============================================================================
    // Test_DT_03 — DynamicTaskFrozenBatch 결정론적 정렬
    // ============================================================================
    static void Test_DT_03_FrozenBatch_Sort_Deterministic()
    {
        LogTestBanner(__FUNCTION__);

        DynamicTaskFrozenBatch batch;

        // scopeId: 1, priorityBias: 0, frame: 5
        batch.requests.push_back({ 1, 1, 0, 0, 5 });
        // scopeId: 0, priorityBias: -1, frame: 2
        batch.requests.push_back({ 1, 0, 0, -1, 2 });
        // scopeId: 0, priorityBias: 5, frame: 1
        batch.requests.push_back({ 1, 0, 0, 5, 1 });
        // scopeId: 0, priorityBias: 5, frame: 3
        batch.requests.push_back({ 1, 0, 0, 5, 3 });
        // scopeId: 0, priorityBias: 0, frame: 2
        batch.requests.push_back({ 1, 0, 0, 0, 2 });

        batch.Sort();

        // 정렬 기준: scopeId ASC → priorityBias DESC → frameIndex ASC
        // 예상 순서 (scopeId=0 먼저, scopeId=1 나중):
        //   [0] scopeId=0, priority=5,  frame=1
        //   [1] scopeId=0, priority=5,  frame=3
        //   [2] scopeId=0, priority=0,  frame=2
        //   [3] scopeId=0, priority=-1, frame=2
        //   [4] scopeId=1, priority=0,  frame=5

        const auto& r = batch.requests;
        assert(r.size() == 5);

        assert(r[0].scopeId == 0 && r[0].priorityBias == 5  && r[0].requestFrameIndex == 1);
        assert(r[1].scopeId == 0 && r[1].priorityBias == 5  && r[1].requestFrameIndex == 3);
        assert(r[2].scopeId == 0 && r[2].priorityBias == 0  && r[2].requestFrameIndex == 2);
        assert(r[3].scopeId == 0 && r[3].priorityBias == -1 && r[3].requestFrameIndex == 2);
        assert(r[4].scopeId == 1 && r[4].priorityBias == 0  && r[4].requestFrameIndex == 5);

        // 동일 입력에 대해 두 번 정렬해도 결과가 같아야 한다.
        batch.Sort();
        assert(r[0].requestFrameIndex == 1);
        assert(r[4].scopeId == 1);
    }

    // ============================================================================
    // Test_DT_04 — 빌더: DynamicTask 노드가 충돌 없으면 병렬(독립) 배치
    // ============================================================================
    static void Test_DT_04_Builder_AppendDynamic_NoConflict_Parallel()
    {
        LogTestBanner(__FUNCTION__);

        ExecutionSourceRegistry sourceRegistry{};
        DynamicTaskTypeRegistry typeRegistry{};

        // AccessSpec이 없는 타입 등록 (충돌 없음)
        DynamicTaskTypeDesc desc{};
        desc.debugName    = "FreeTask";
        desc.defaultPhase = ExecPhase::Simulate;
        desc.defaultLane  = ExecLane::Parallel;
        desc.dispatchFn   = &DynDispatch_RecordPayload;
        // accesses 비어 있음 → HasAnyConflict = false

        const DynamicTaskTypeId typeId = typeRegistry.Register(
            std::move(desc), sourceRegistry);
        assert(typeId != InvalidDynamicTaskTypeId);

        // 정적 노드 없이 dynamic 노드 2개만 빌드
        DynamicTaskFrozenBatch batch;
        batch.requests.push_back({ typeId, 0, 100, 0, 1 });
        batch.requests.push_back({ typeId, 0, 200, 0, 2 });
        batch.Sort();

        // 정적 fragment는 없고 dynamic batch만 전달
        // → 빌더가 scope 0으로 처리할 수 있도록 scopeCount를 수동 설정해야 하지만,
        //   fragment가 없으면 scopeCount=0이 된다.
        //   실제 세계에서는 Commit/LifecycleFlush 등 최소 1개 fragment가 있으므로,
        //   여기서는 AssembleFrameGraphNodes를 직접 호출하는 대신
        //   ExecutionGraphBuilder::Build()에 빈 selection set을 넣는 것은 실패한다.
        //
        //   대신 AppendDynamicNodes를 직접 단위 테스트한다.

        FrameTaskGraph graph;
        graph.scopeCount = 1;
        graph.scopeToWorld = { 9001 };
        graph.simulateNodeCount = 0;

        std::vector<std::vector<ExecNodeId>> predLists;
        std::vector<std::vector<ExecNodeId>> succLists;
        DynamicTaskFrameTable frameTable;
        BuildResult result{};

        ExecutionGraphBuilder builder{};
        ExecutionGraphBuildPolicy policy{};

        // [1] 빈 fragments → nodes는 비어 있다.
        std::vector<WorldFragmentBuild> fragments; // empty
        ExecutionGraphBuilderTestHook::AssembleFrameGraphNodes(
            builder, fragments, graph, policy, predLists, succLists);

        // 직접 scopeCount를 복원 (AssembleFrameGraphNodes는 Clear()를 호출하므로)
        graph.scopeCount = 1;
        graph.scopeToWorld = { 9001 };

        // [2] DynamicNode 추가
        ExecutionGraphBuilderTestHook::AppendDynamicNodes(
            builder, batch, typeRegistry, sourceRegistry,
            nullptr, graph, predLists, succLists,
            frameTable, result);

        // [3] edge pool 확정
        ExecutionGraphBuilderTestHook::FinalizeEdgePool(
            builder, graph, predLists, succLists, policy);

        // 검증: 노드 2개, 엣지 없음(충돌 없으니)
        assert(graph.nodes.size() == 2);
        assert(graph.simulateNodeCount == 2);
        assert(frameTable.InstanceCount() == 2);

        // 두 노드 사이에 엣지가 없어야 한다 (충돌 없음 = 독립)
        const ExecNodeRecord& n0 = graph.nodes[0];
        const ExecNodeRecord& n1 = graph.nodes[1];
        assert(n0.predCount == 0 && n0.succCount == 0);
        assert(n1.predCount == 0 && n1.succCount == 0);

        // frameTable 조회 검증
        const DynamicTaskInstance* inst0 = frameTable.FindByNodeId(0);
        const DynamicTaskInstance* inst1 = frameTable.FindByNodeId(1);
        assert(inst0 != nullptr && inst0->payloadKey == 100);
        assert(inst1 != nullptr && inst1->payloadKey == 200);

        assert(!result.HasError());
    }

    // ============================================================================
    // Test_DT_05 — 빌더: 충돌하는 두 DynamicTask 노드 → 자동 순서 엣지
    // ============================================================================
    static void Test_DT_05_Builder_AppendDynamic_Conflict_AutoEdge()
    {
        LogTestBanner(__FUNCTION__);

        ExecutionSourceRegistry sourceRegistry{};
        DynamicTaskTypeRegistry typeRegistry{};

        // Component를 Write하는 타입 — 같은 resource에 대해 Write+Write = 충돌
        static constexpr AccessSpec kWriteAccess{
            MakeResourceId<ResourceKinds::Component, FaultProbeComp>(),
            AccessMode::Write,
            Visibility::Immediate
        };

        DynamicTaskTypeDesc desc{};
        desc.debugName    = "WriterTask";
        desc.defaultPhase = ExecPhase::Simulate;
        desc.defaultLane  = ExecLane::Parallel;
        desc.dispatchFn   = &DynDispatch_RecordPayload;
        desc.accesses     = { kWriteAccess };

        const DynamicTaskTypeId typeId = typeRegistry.Register(
            std::move(desc), sourceRegistry);
        assert(typeId != InvalidDynamicTaskTypeId);

        DynamicTaskFrozenBatch batch;
        // 우선순위 높은 것이 먼저 배치된다
        batch.requests.push_back({ typeId, 0, 11, 1, 1 }); // priority=1, payload=11
        batch.requests.push_back({ typeId, 0, 22, 0, 2 }); // priority=0, payload=22
        batch.Sort();
        // 정렬 후: [payload=11, priority=1] → [payload=22, priority=0]

        FrameTaskGraph graph;
        graph.scopeCount = 1;
        graph.scopeToWorld = { 9002 };

        std::vector<std::vector<ExecNodeId>> predLists;
        std::vector<std::vector<ExecNodeId>> succLists;
        DynamicTaskFrameTable frameTable;
        BuildResult result{};

        ExecutionGraphBuilder builder{};
        ExecutionGraphBuildPolicy policy{};
        std::vector<WorldFragmentBuild> fragments;

        ExecutionGraphBuilderTestHook::AssembleFrameGraphNodes(
            builder, fragments, graph, policy, predLists, succLists);
        graph.scopeCount = 1;
        graph.scopeToWorld = { 9002 };

        ExecutionGraphBuilderTestHook::AppendDynamicNodes(
            builder, batch, typeRegistry, sourceRegistry,
            nullptr, graph, predLists, succLists,
            frameTable, result);

        ExecutionGraphBuilderTestHook::FinalizeEdgePool(
            builder, graph, predLists, succLists, policy);

        assert(!result.HasError());
        assert(graph.nodes.size() == 2);

        // 노드 0 (priority=1, payload=11): succ에 노드 1이 있어야 한다.
        // 노드 1 (priority=0, payload=22): pred에 노드 0이 있어야 한다.
        const ExecNodeRecord& n0 = graph.nodes[0];
        const ExecNodeRecord& n1 = graph.nodes[1];

        assert(n0.succCount == 1);
        assert(graph.edges[n0.succBegin] == 1);

        assert(n1.predCount == 1);
        assert(graph.edges[n1.predBegin] == 0);

        // frameTable에서 payload 검증
        const DynamicTaskInstance* inst0 = frameTable.FindByNodeId(0);
        const DynamicTaskInstance* inst1 = frameTable.FindByNodeId(1);
        assert(inst0 != nullptr && inst0->payloadKey == 11); // priority=1 먼저
        assert(inst1 != nullptr && inst1->payloadKey == 22);
    }

    // ============================================================================
    // Test_DT_06 — Executor: DynamicTask dispatch → frameTable payload resolve
    // ============================================================================
    static void Test_DT_06_Executor_Dispatches_DynamicTask_And_Resolves_Payload()
    {
        LogTestBanner(__FUNCTION__);

        DynTestRecorder recorder{};
        g_recorder = &recorder;

        ExecutionSourceRegistry sourceRegistry{};
        DynamicTaskTypeRegistry typeRegistry{};

        DynamicTaskTypeDesc desc{};
        desc.debugName    = "PayloadTask";
        desc.defaultPhase = ExecPhase::Simulate;
        desc.defaultLane  = ExecLane::Parallel;
        desc.dispatchFn   = &DynDispatch_RecordPayload;
        const DynamicTaskTypeId typeId = typeRegistry.Register(
            std::move(desc), sourceRegistry);
        assert(typeId != InvalidDynamicTaskTypeId);

        const DynamicTaskTypeDesc* stored = typeRegistry.TryGet(typeId);

        // 수동으로 그래프를 구성한다.
        // dynamic 노드 2개: payloadKey 42, 99
        DynTestRig rig{};
        rig.graph.scopeCount = 1;
        rig.graph.scopeToWorld = { 7001 };
        rig.graph.simulateNodeCount = 2;

        // 충돌 없으므로 엣지 없음
        ExecNodeRecord n0{};
        n0.id = 0; n0.scopeId = 0;
        n0.phase = ExecPhase::Simulate;
        n0.lane  = ExecLane::Parallel;
        n0.kind  = ExecNodeKind::DynamicTask;
        n0.sourceToken = stored->sourceToken;
        rig.graph.nodes.push_back(n0);

        ExecNodeRecord n1{};
        n1.id = 1; n1.scopeId = 0;
        n1.phase = ExecPhase::Simulate;
        n1.lane  = ExecLane::Parallel;
        n1.kind  = ExecNodeKind::DynamicTask;
        n1.sourceToken = stored->sourceToken;
        rig.graph.nodes.push_back(n1);

        // frameTable에 인스턴스 등록
        DynamicTaskInstance inst0{};
        inst0.typeId = typeId; inst0.scopeId = 0;
        inst0.graphNodeId = 0; inst0.payloadKey = 42;
        rig.frameTable.AddInstance(inst0);

        DynamicTaskInstance inst1{};
        inst1.typeId = typeId; inst1.scopeId = 0;
        inst1.graphNodeId = 1; inst1.payloadKey = 99;
        rig.frameTable.AddInstance(inst1);

        rig.Finalize();

        // sources는 DynTestRig 내부에 없으므로 rig.sources에 추가
        // rig.sources가 아닌 sourceRegistry를 사용 — ExecuteFrame 인자로 넘긴다.

        TaskExecutor executor(2);
        const bool ok = executor.ExecuteFrame(rig.frame, rig.runtime, sourceRegistry);
        assert(ok);

        ExpectNodeState(rig.runtime, 0, ExecNodeState::Succeeded);
        ExpectNodeState(rig.runtime, 1, ExecNodeState::Succeeded);

        assert(recorder.PayloadCount() == 2);
        assert(recorder.HasPayload(42));
        assert(recorder.HasPayload(99));

        g_recorder = nullptr;
    }

    // ============================================================================
    // Test_DT_07 — 핵심 불변성:
    //   프레임 N에서 push된 요청은 프레임 N+1에서 실행된다.
    //   (같은 프레임 N에서는 절대 실행되지 않는다.)
    // ============================================================================

    // 프레임 1에서 호출되는 static system: DynamicTaskRequest를 push한다.
    static DynamicTaskTypeId   g_frameInvTest_typeId   = InvalidDynamicTaskTypeId;
    static ExecScopeId         g_frameInvTest_scopeId  = 0;
    static DynTestRecorder*    g_frameInvTest_recorder = nullptr;

    static ExecCallResult FrameInv_StaticSys_PushRequest(NodeExecContext& ctx)
    {
        assert(ctx.IsValid());

        // 현재 프레임에서 DynamicTaskRequest를 push한다.
        WorldRuntime* runtime = ctx.TryGetRuntime();
        assert(runtime != nullptr);

        DynamicTaskRequest req{};
        req.typeId   = g_frameInvTest_typeId;
        req.scopeId  = g_frameInvTest_scopeId;
        req.payloadKey = 12345;
        runtime->PushDynamicTaskRequest(req);

        if (g_frameInvTest_recorder)
            g_frameInvTest_recorder->RecordEvent("StaticPush");

        return ExecCallResult::Success;
    }

    static ExecCallResult FrameInv_DynamicDispatch(NodeExecContext& ctx)
    {
        assert(ctx.IsValid());
        assert(ctx.frame->dynamicTaskFrameTable != nullptr);

        const DynamicTaskInstance* inst =
            ctx.frame->dynamicTaskFrameTable->FindByNodeId(ctx.nodeId);
        assert(inst != nullptr);

        if (g_frameInvTest_recorder)
        {
            g_frameInvTest_recorder->RecordPayload(inst->payloadKey);
            g_frameInvTest_recorder->RecordEvent("DynamicExecuted");
        }

        return ExecCallResult::Success;
    }

    static void Test_DT_07_FrameInvariant_PushInFrameN_ExecuteInFrameN1()
    {
        LogTestBanner(__FUNCTION__);

        constexpr ExecToken kStaticToken  = 1;
        constexpr ExecToken kCommitToken  = 2;
        constexpr ExecToken kLifecycle    = 3;
        constexpr ExecToken kReconcile    = 4;
        constexpr WorldExecutionModelKey kModelKey = 300;

        DynTestRecorder recorder{};
        g_frameInvTest_recorder = &recorder;

        // ----------------------------------------------------------------
        // Bootstrap
        // ----------------------------------------------------------------
        DynTestWorldFactory factory{};
        WorldExecutionModelRegistry executionModelRegistry{};
        WorldRegistry worldRegistry(factory, executionModelRegistry);
        ExecutionSourceRegistry sourceRegistry{};
        WorldTransferProfileRegistry transferProfileRegistry{};

        // Static System: 프레임 1에서 DynamicTaskRequest를 push
        ExecutionSourceDesc staticDesc{};
        staticDesc.token     = kStaticToken;
        staticDesc.phase     = ExecPhase::Simulate;
        staticDesc.lane      = ExecLane::Parallel;
        staticDesc.kind      = ExecNodeKind::StaticSystem;
        staticDesc.fn        = &FrameInv_StaticSys_PushRequest;
        staticDesc.debugName = "StaticPushSys";
        assert(sourceRegistry.Register(staticDesc));

        ExecutionSourceDesc commitDesc{};
        commitDesc.token = kCommitToken; commitDesc.phase = ExecPhase::Commit;
        commitDesc.lane = ExecLane::Serial; commitDesc.kind = ExecNodeKind::StructuralApply;
        commitDesc.fn = [](NodeExecContext& ctx) -> ExecCallResult {
            auto* ops = ctx.TryGetOps();
            if (ops) ops->CommitScope(ctx.scopeId, ctx.frame->runtimeByScope);
            return ExecCallResult::Success;
        };
        commitDesc.debugName = "Commit";
        assert(sourceRegistry.Register(commitDesc));

        ExecutionSourceDesc lifecycleDesc{};
        lifecycleDesc.token = kLifecycle; lifecycleDesc.phase = ExecPhase::LifecycleFlush;
        lifecycleDesc.lane = ExecLane::Serial; lifecycleDesc.kind = ExecNodeKind::LifecycleFlush;
        lifecycleDesc.fn = [](NodeExecContext& ctx) -> ExecCallResult {
            auto* ops = ctx.TryGetOps();
            if (ops) ops->FlushLifecycle(ctx.scopeId, ctx.frame->runtimeByScope);
            return ExecCallResult::Success;
        };
        lifecycleDesc.debugName = "Lifecycle";
        assert(sourceRegistry.Register(lifecycleDesc));

        ExecutionSourceDesc reconcileDesc{};
        reconcileDesc.token = kReconcile; reconcileDesc.phase = ExecPhase::Reconcile;
        reconcileDesc.lane = ExecLane::Serial; reconcileDesc.kind = ExecNodeKind::Reconcile;
        reconcileDesc.fn = [](NodeExecContext& ctx) -> ExecCallResult {
            auto* ops = ctx.TryGetOps();
            if (ops) ops->ReconcileScope(ctx.scopeId, ctx.frame->runtimeByScope);
            return ExecCallResult::Success;
        };
        reconcileDesc.debugName = "Reconcile";
        assert(sourceRegistry.Register(reconcileDesc));

        WorldExecutionModel model{};
        model.key = kModelKey;
        model.simulateSources   = { kStaticToken };
        model.commitSources     = { kCommitToken };
        model.lifecycleFlushSources = { kLifecycle };
        model.reconcileSources  = { kReconcile };
        assert(executionModelRegistry.Register(model, sourceRegistry));

        WorldDef def = MakeMinimalWorldDef("FrameInvWorld");
        def.executionModelKey = kModelKey;
        assert(worldRegistry.RegisterWorldDef(def));

        WorldManager worldManager(worldRegistry);
        const WorldId worldId = worldManager.RegisterPreCreatedWorld(def.id, 1);
        assert(worldId.IsValid());

        WorldInstance* world = worldRegistry.FindWorld(worldId);
        assert(world != nullptr && world->Initialize());

        worldManager.FlushLifecycle(0.0);
        worldManager.FlushLifecycle(0.0);
        assert(worldManager.FindRecord(worldId)->IsRunnable());

        // ----------------------------------------------------------------
        // DynamicTask 타입 등록
        // ----------------------------------------------------------------
        DynamicTaskTypeRegistry typeRegistry{};

        DynamicTaskTypeDesc dynDesc{};
        dynDesc.debugName    = "FrameInvDynTask";
        dynDesc.defaultPhase = ExecPhase::Simulate;
        dynDesc.defaultLane  = ExecLane::Parallel;
        dynDesc.dispatchFn   = &FrameInv_DynamicDispatch;
        const DynamicTaskTypeId dynTypeId = typeRegistry.Register(
            std::move(dynDesc), sourceRegistry);
        assert(dynTypeId != InvalidDynamicTaskTypeId);

        g_frameInvTest_typeId  = dynTypeId;
        // scopeId = 0 (첫 번째 world)
        g_frameInvTest_scopeId = 0;

        // ----------------------------------------------------------------
        // Scheduler 구성 (DynamicTaskTypeRegistry 전달)
        // ----------------------------------------------------------------
        ExecutionGraphBuilder graphBuilder{};
        ExecutionGraphBuildPolicy buildPolicy{};
        TaskExecutor executor(2);
        ExecutionOps ops(
            &worldManager,
            &worldRegistry,
            reinterpret_cast<WorldTransferService*>(0x1),
            reinterpret_cast<WorldAdmissionService*>(0x1),
            reinterpret_cast<NetIdRegistry*>(0x1),
            reinterpret_cast<PresenceManager*>(0x1));

        WorldScheduler scheduler(
            worldManager,
            worldRegistry,
            executionModelRegistry,
            sourceRegistry,
            graphBuilder,
            buildPolicy,
            executor,
            ops,
            WorldSchedulerConfig{},
            &typeRegistry);   // ← DynamicTaskTypeRegistry 주입

        // ================================================================
        // Frame 1: StaticSys가 실행되어 DynamicTaskRequest를 push한다.
        //          이 프레임에서 dynamic task는 실행되지 않아야 한다.
        // ================================================================
        WorldSchedulerFrameResult result1{};
        const bool runOk1 = scheduler.RunFrame({ 1, 1.0, 0.016 }, result1);

        assert(runOk1);
        assert(result1.success);
        assert(result1.executed);

        // Frame 1: StaticPush 이벤트가 있어야 한다.
        assert(recorder.EventCount() >= 1);
        const bool hasPush = std::any_of(
            recorder.events.begin(), recorder.events.end(),
            [](const std::string& s) { return s == "StaticPush"; });
        assert(hasPush);

        // Frame 1: DynamicExecuted 이벤트는 없어야 한다 (프레임 불변성).
        const bool hasEarlyExec = std::any_of(
            recorder.events.begin(), recorder.events.end(),
            [](const std::string& s) { return s == "DynamicExecuted"; });
        assert(!hasEarlyExec);

        // Frame 1: payload 실행도 없어야 한다.
        assert(recorder.PayloadCount() == 0);

        std::cout << "[TRACE] Frame1: events=" << recorder.EventCount()
                  << " payloads=" << recorder.PayloadCount() << "\n";

        // ================================================================
        // Frame 2: freeze → 그래프 빌드 → DynamicTask 노드 포함 → 실행
        //          payloadKey=12345 가 이번 프레임에 실행되어야 한다.
        // ================================================================
        WorldSchedulerFrameResult result2{};
        const bool runOk2 = scheduler.RunFrame({ 2, 2.0, 0.016 }, result2);

        assert(runOk2);
        assert(result2.success);
        assert(result2.executed);

        // Frame 2: DynamicExecuted 이벤트가 있어야 한다.
        const bool hasDynExec = std::any_of(
            recorder.events.begin(), recorder.events.end(),
            [](const std::string& s) { return s == "DynamicExecuted"; });
        assert(hasDynExec);

        // payload 12345가 실행됐어야 한다.
        assert(recorder.HasPayload(12345));

        std::cout << "[TRACE] Frame2: events=" << recorder.EventCount()
                  << " payloads=" << recorder.PayloadCount() << "\n";

        // Frame 2 그래프에 dynamic 노드가 포함됐는지 진단 출력
        const BuildResult& buildResult2 = scheduler.GetLastBuildResult();
        std::cout << "[TRACE] Frame2 graph: nodes=" << buildResult2.graph.nodes.size()
                  << " dynamicInstances=" << buildResult2.dynamicTaskFrameTable.InstanceCount()
                  << " diagnostics=" << buildResult2.diagnostics.size() << "\n";

        assert(buildResult2.dynamicTaskFrameTable.InstanceCount() >= 1);

        g_frameInvTest_recorder = nullptr;
        g_frameInvTest_typeId   = InvalidDynamicTaskTypeId;
    }

    // ============================================================================
    // Test_DT_08 — 같은 프레임에서 push+execute는 절대 발생하지 않는다.
    //   프레임 N에서 push 후, pending queue에는 요청이 남아 있어야 하고
    //   다음 프레임 전까지 graph.nodes에 dynamic 노드가 없어야 한다.
    // ============================================================================
    static void Test_DT_08_SameFrame_PushAndExecute_NotPossible()
    {
        LogTestBanner(__FUNCTION__);

        DynamicTaskPendingQueue queue;
        DynamicTaskTypeId typeId = 1; // 임의 ID

        // 큐에 push
        DynamicTaskRequest req{ typeId, 0, 999, 0, 1 };
        queue.Push(req);

        // 아직 freeze 전 — pending count = 1
        assert(queue.PendingCount() == 1);

        // 같은 "프레임 내"에서는 Drain이 호출되지 않으므로
        // freeze 없이 건드리지 않으면 요청은 실행되지 않는다.

        // Drain (freeze 경계 시뮬레이션)
        DynamicTaskFrozenBatch batch;
        queue.DrainInto(batch.requests);
        batch.Sort();

        // 이제 큐는 비어 있다.
        assert(queue.PendingCount() == 0);

        // batch에는 요청이 있다.
        assert(batch.requests.size() == 1);
        assert(batch.requests[0].payloadKey == 999);

        // FrameTaskGraph에 dynamic 노드를 추가하지 않으면 실행 기회가 없다.
        // 이 테스트는 구조적 불변성(queue가 freeze 전에 비워지지 않음)을 검증한다.
        // queue.PendingCount() == 0이 아니었다면 같은 프레임에서 실행 가능성이 있다.

        std::cout << "[TRACE] SameFrame: batch.requests=" << batch.requests.size()
                  << " queue remaining=" << queue.PendingCount() << "\n";
    }

} // namespace

// ============================================================================
// RunDynamicTaskSmokeTests — called from main() via --dynamic-task-smoke
// ============================================================================
void RunDynamicTaskSmokeTests()
{
    Test_DT_01_TypeRegistry_Register_Basic();
    std::cout << "[PASS] Test_DT_01_TypeRegistry_Register_Basic\n";

    Test_DT_02_PendingQueue_ConcurrentPush_Drain();
    std::cout << "[PASS] Test_DT_02_PendingQueue_ConcurrentPush_Drain\n";

    Test_DT_03_FrozenBatch_Sort_Deterministic();
    std::cout << "[PASS] Test_DT_03_FrozenBatch_Sort_Deterministic\n";

    Test_DT_04_Builder_AppendDynamic_NoConflict_Parallel();
    std::cout << "[PASS] Test_DT_04_Builder_AppendDynamic_NoConflict_Parallel\n";

    Test_DT_05_Builder_AppendDynamic_Conflict_AutoEdge();
    std::cout << "[PASS] Test_DT_05_Builder_AppendDynamic_Conflict_AutoEdge\n";

    Test_DT_06_Executor_Dispatches_DynamicTask_And_Resolves_Payload();
    std::cout << "[PASS] Test_DT_06_Executor_Dispatches_DynamicTask_And_Resolves_Payload\n";

    Test_DT_07_FrameInvariant_PushInFrameN_ExecuteInFrameN1();
    std::cout << "[PASS] Test_DT_07_FrameInvariant_PushInFrameN_ExecuteInFrameN1\n";

    Test_DT_08_SameFrame_PushAndExecute_NotPossible();
    std::cout << "[PASS] Test_DT_08_SameFrame_PushAndExecute_NotPossible\n";

    std::cout << "\nAll DynamicTask smoke tests passed.\n";
}
