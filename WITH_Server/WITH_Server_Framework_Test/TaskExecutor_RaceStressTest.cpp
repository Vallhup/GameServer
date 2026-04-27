#include "pch.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "AutoSystemBridge.h"
#include "ExecutionContextTypes.h"
#include "ExecutionGraphBuildPolicy.h"
#include "ExecutionGraphBuilder.h"
#include "ExecutionOps.h"
#include "ExecutionRuntimeTypes.h"
#include "SystemMetaStorage.h"
#include "TaskExecutor.h"
#include "WorldDef.h"
#include "WorldExecutionModelTypes.h"
#include "WorldFrameSelectionTypes.h"
#include "WorldRuntime.h"

namespace
{
    // =========================================================================
    // 메모리 수준 data race 감지 — 추후 확장 과제
    // =========================================================================
    //
    // 현재 검증 방식의 한계
    // ----------------------
    // 이 테스트의 모든 데이터 접근 패턴 검증은 "논리적(behavioral)" 수준에서
    // 이루어진다. 즉, 시스템 body 안에서 mutex/atomic 카운터로 "지금 이 world에
    // 동시에 writer가 몇 명 실행 중인가"를 세는 방식이다.
    //
    // 이 방식은:
    //  (a) 스케줄러가 잘못된 edge를 생성하거나 edge를 누락할 경우 감지하지만,
    //  (b) 올바른 edge가 있더라도 executor의 메모리 순서 보장(happens-before)이
    //      무너졌을 때 발생하는 실제 메모리 레벨 race는 감지하지 못한다.
    //
    //  예: writer가 store(release)를 했고 reader가 load(acquire)를 했는데,
    //  executor 내부의 atomic CAS chain이 실제 happens-before를 전달하지 못한다면
    //  reader는 stale 값을 볼 수 있다. 현재 테스트는 이를 잡지 못한다.
    //
    // 추후 확장 방향 A — ThreadSanitizer(TSan) 통합
    // -----------------------------------------------
    // MSVC: /fsanitize=address 가 ASan만 지원하며 TSan은 미지원(2024 기준).
    // clang-cl 또는 GCC 빌드 변형에서 -fsanitize=thread 를 활성화한다.
    //
    // 적용 방법:
    //  1. TSan 빌드 구성을 추가한다 (CMakeLists 또는 vcxproj 변형).
    //  2. 시스템 body 안의 ImmediateProbeComp 접근을 non-atomic raw 포인터로
    //     직접 읽고 쓴다 (현재도 그러하다).
    //  3. 직렬화 없이 두 writer 또는 writer+reader가 동시에 실행되면 TSan이
    //     RACE 를 리포트한다.
    //  4. CI 파이프라인에서 TSan 빌드를 별도 job으로 실행한다.
    //
    // 추후 확장 방향 B — Self-contained Sequence-Lock 기반 감지
    // ----------------------------------------------------------
    // TSan을 쓰지 못하는 환경(MSVC 전용)에서는 seqlock 패턴으로 race를 감지할 수
    // 있다.
    //
    //  struct RaceProbe {
    //      std::atomic<uint32_t> seq{ 0 };   // 홀수 = 쓰기 중
    //      uint64_t              value{ 0 };  // non-atomic intentional
    //  };
    //
    //  Writer:
    //    probe.seq.fetch_add(1, release);     // seq = 홀수
    //    probe.value = newValue;              // non-atomic write
    //    probe.seq.fetch_add(1, release);     // seq = 짝수
    //
    //  Reader:
    //    uint32_t s0 = probe.seq.load(acquire);
    //    if (s0 & 1) Fail("reader started while writer active");
    //    uint64_t v = probe.value;
    //    uint32_t s1 = probe.seq.load(acquire);
    //    if (s0 != s1) Fail("torn read: writer ran concurrently with reader");
    //
    // 이 기법은 두 이벤트 사이의 "쓰기가 없는 상태"를 seq 짝수/홀수로 표현하며,
    // 읽기 도중 seq 가 변경됐다면 동시 쓰기가 있었음을 의미한다.
    // 단, x86-64 에서는 word-aligned 64bit 읽기가 본질적으로 atomic이므로
    // 실제 torn-read 보다는 happens-before 위반을 감지하는 효과가 크다.
    //
    // 추후 확장 방향 C — per-component 접근 epoch 검증
    // -------------------------------------------------
    // 각 컴포넌트에 "마지막으로 쓴 frame 번호"를 atomic으로 붙이고,
    // reader 실행 시 컴포넌트의 frame 번호가 현재 frame 과 일치하는지 확인한다.
    // 이는 "이 컴포넌트가 이 frame 에서 최소 한 번 쓰였음"을 검증한다.
    // 현재 ImmediateProbeComp 값 검증이 이 아이디어의 약한 버전이다.
    // =========================================================================

    constexpr WorldExecutionModelKey kRaceStressModelKey = 901;
    constexpr int kSnapshotReaderCount = 16;
    constexpr int kSnapshotDeferredWriterCount = 1;
    constexpr int kDeferredRecorderCount = 16;
    constexpr int kCommandBufferRecorderCount = 4;
    constexpr int kImmediateReaderCount = 6;
    constexpr int kImmediateWriterCount = 2;

    struct SnapshotProbeComp final : Component
    {
        uint64_t value{ 0 };
    };

    struct ImmediateProbeComp final : Component
    {
        uint64_t value{ 0 };
    };

    template<int Id>
    struct DeferredProbeComp final : Component
    {
        uint64_t value{ 0 };
    };

    template<int Id>
    struct CommandBufferProbeComp final : Component
    {
        uint64_t value{ 0 };
    };

    struct StressOptions
    {
        uint64_t frames{ 1000 };
        uint32_t workers{ 0 };
        uint32_t worlds{ 2 };
        bool infinite{ false };
        bool verbose{ false };
        // SPEC-EXEC-QUEUE-001 §개선방향 §6:
        // strict mode에서는 invariant counter 중 하나라도 증가하면 즉시 fail.
        bool strict{ false };
    };

    struct RuntimeTarget
    {
        WorldRuntime* runtime{ nullptr };
        Entity commonTarget{ Entity::Null() };
        std::array<Entity, kDeferredRecorderCount> deferredTargets{};
        std::array<Entity, kCommandBufferRecorderCount> commandBufferTargets{};
    };

    struct StressState
    {
        static constexpr size_t kMissingRuntimeIndex = static_cast<size_t>(-1);

        std::atomic<uint64_t> currentFrame{ 0 };
        std::atomic<uint64_t> frameStartRecorded{ 0 };
        std::atomic<uint64_t> frameStartApplied{ 0 };

        std::atomic<uint64_t> recorded{ 0 };
        std::atomic<uint64_t> applied{ 0 };
        std::atomic<uint64_t> failures{ 0 };

        std::atomic<int> activeSnapshotReaders{ 0 };
        std::atomic<int> maxSnapshotReaders{ 0 };
        // Gap 2: per-world snapshot reader concurrency tracking.
        // global maxSnapshotReaders >= 2 는 서로 다른 world의 reader 2개로도 충족될 수
        // 있으므로, 같은 world 내 병렬성을 별도로 추적한다.
        int maxSnapshotReadersPerWorld{ 0 };

        std::atomic<int> activeMutationRecorders{ 0 };
        std::atomic<int> maxMutationRecorders{ 0 };

        std::atomic<int> activeCommandBufferRecorders{ 0 };
        std::atomic<int> maxCommandBufferRecorders{ 0 };
        int maxCommandBufferRecordersPerWorld{ 0 };

        std::atomic<int> immediateReaders{ 0 };
        std::atomic<int> immediateWriters{ 0 };
        std::atomic<int> maxImmediateReaders{ 0 };
        int maxImmediateReadersPerWorld{ 0 };

        std::atomic<uint64_t> commitProbeCount{ 0 };
        std::atomic<uint64_t> lifecycleProbeCount{ 0 };
        std::atomic<uint64_t> reconcileProbeCount{ 0 };

        uint64_t expectedRecordsPerFrame{ 0 };

        std::mutex failureMutex;
        std::mutex commandBufferMutex;
        std::mutex immediateMutex;
        std::mutex snapshotReaderMutex;  // Gap 2: per-world snapshot reader 추적용
        std::vector<std::string> failureMessages;
        std::vector<RuntimeTarget> targets;
        std::vector<int> activeCommandBufferRecordersByWorld;
        std::vector<int> activeImmediateReadersByWorld;
        std::vector<int> activeImmediateWritersByWorld;
        std::vector<int> activeSnapshotReadersByWorld;  // Gap 2

        // Gap 3: ImmediateWriter가 실행 후 실제로 쓴 값을 per-world로 추적한다.
        // atomic<uint64_t>은 move 불가이므로 uint64_t + std::atomic_ref<> 패턴을 사용한다.
        // targets 확정 후 resize되며, 이후 크기는 변경되지 않는다.
        std::vector<uint64_t> lastImmediateValueByWorld;

        void Fail(std::string message)
        {
            failures.fetch_add(1, std::memory_order_acq_rel);
            std::lock_guard lock{ failureMutex };
            if (failureMessages.size() < 32)
                failureMessages.push_back(std::move(message));
        }

        void UpdateMax(std::atomic<int>& target, int value)
        {
            int previous = target.load(std::memory_order_relaxed);
            while (previous < value &&
                !target.compare_exchange_weak(
                    previous,
                    value,
                    std::memory_order_relaxed))
            {
            }
        }

        const RuntimeTarget* TargetsFor(WorldRuntime& runtime) const
        {
            for (const RuntimeTarget& target : targets)
            {
                if (target.runtime == &runtime)
                    return &target;
            }
            return nullptr;
        }

        size_t RuntimeIndexFor(WorldRuntime& runtime) const
        {
            for (size_t index = 0; index < targets.size(); ++index)
            {
                if (targets[index].runtime == &runtime)
                    return index;
            }
            return kMissingRuntimeIndex;
        }

        // Gap 2: per-world snapshot reader 진입/이탈.
        // global activeSnapshotReaders 는 atomic으로 별도 관리하고,
        // per-world 카운트는 snapshotReaderMutex 아래에서 업데이트한다.
        int EnterSnapshotReader(WorldRuntime& runtime)
        {
            const int activeTotal =
                activeSnapshotReaders.fetch_add(1, std::memory_order_acq_rel) + 1;
            UpdateMax(maxSnapshotReaders, activeTotal);

            std::lock_guard lock{ snapshotReaderMutex };
            const size_t index = RuntimeIndexFor(runtime);
            if (index == kMissingRuntimeIndex ||
                index >= activeSnapshotReadersByWorld.size())
            {
                Fail("SnapshotReader missing per-world runtime counter");
                return 0;
            }

            const int activeInWorld = ++activeSnapshotReadersByWorld[index];
            maxSnapshotReadersPerWorld =
                std::max(maxSnapshotReadersPerWorld, activeInWorld);
            return activeInWorld;
        }

        void LeaveSnapshotReader(WorldRuntime& runtime)
        {
            {
                std::lock_guard lock{ snapshotReaderMutex };
                const size_t index = RuntimeIndexFor(runtime);
                if (index == kMissingRuntimeIndex ||
                    index >= activeSnapshotReadersByWorld.size())
                {
                    Fail("SnapshotReader missing per-world runtime counter on leave");
                }
                else
                {
                    --activeSnapshotReadersByWorld[index];
                }
            }

            activeSnapshotReaders.fetch_sub(1, std::memory_order_acq_rel);
        }

        int EnterCommandBufferRecorder(WorldRuntime& runtime)
        {
            const int activeTotal =
                activeCommandBufferRecorders.fetch_add(1, std::memory_order_acq_rel) + 1;
            UpdateMax(maxCommandBufferRecorders, activeTotal);

            std::lock_guard lock{ commandBufferMutex };
            const size_t index = RuntimeIndexFor(runtime);
            if (index == kMissingRuntimeIndex ||
                index >= activeCommandBufferRecordersByWorld.size())
            {
                Fail("CommandBuffer recorder missing runtime counter");
                return 0;
            }

            const int activeInWorld = ++activeCommandBufferRecordersByWorld[index];
            maxCommandBufferRecordersPerWorld =
                std::max(maxCommandBufferRecordersPerWorld, activeInWorld);
            return activeInWorld;
        }

        void LeaveCommandBufferRecorder(WorldRuntime& runtime)
        {
            {
                std::lock_guard lock{ commandBufferMutex };
                const size_t index = RuntimeIndexFor(runtime);
                if (index == kMissingRuntimeIndex ||
                    index >= activeCommandBufferRecordersByWorld.size())
                {
                    Fail("CommandBuffer recorder missing runtime counter on leave");
                }
                else
                {
                    --activeCommandBufferRecordersByWorld[index];
                }
            }

            activeCommandBufferRecorders.fetch_sub(1, std::memory_order_acq_rel);
        }

        int EnterImmediateReader(WorldRuntime& runtime)
        {
            const int activeTotal =
                immediateReaders.fetch_add(1, std::memory_order_acq_rel) + 1;
            UpdateMax(maxImmediateReaders, activeTotal);

            std::lock_guard lock{ immediateMutex };
            const size_t index = RuntimeIndexFor(runtime);
            if (index == kMissingRuntimeIndex ||
                index >= activeImmediateReadersByWorld.size())
            {
                Fail("ImmediateReader missing runtime counter");
                return 0;
            }

            if (activeImmediateWritersByWorld[index] != 0)
                Fail("ImmediateReader overlapped an ImmediateWriter in the same world");

            const int activeInWorld = ++activeImmediateReadersByWorld[index];
            maxImmediateReadersPerWorld =
                std::max(maxImmediateReadersPerWorld, activeInWorld);
            return activeInWorld;
        }

        void LeaveImmediateReader(WorldRuntime& runtime)
        {
            {
                std::lock_guard lock{ immediateMutex };
                const size_t index = RuntimeIndexFor(runtime);
                if (index == kMissingRuntimeIndex ||
                    index >= activeImmediateReadersByWorld.size())
                {
                    Fail("ImmediateReader missing runtime counter on leave");
                }
                else
                {
                    --activeImmediateReadersByWorld[index];
                }
            }

            immediateReaders.fetch_sub(1, std::memory_order_acq_rel);
        }

        int EnterImmediateWriter(WorldRuntime& runtime)
        {
            const int activeTotal =
                immediateWriters.fetch_add(1, std::memory_order_acq_rel) + 1;
            (void)activeTotal;

            std::lock_guard lock{ immediateMutex };
            const size_t index = RuntimeIndexFor(runtime);
            if (index == kMissingRuntimeIndex ||
                index >= activeImmediateWritersByWorld.size())
            {
                Fail("ImmediateWriter missing runtime counter");
                return 0;
            }

            const int activeInWorld = ++activeImmediateWritersByWorld[index];
            if (activeInWorld != 1)
                Fail("ImmediateWriter overlapped another ImmediateWriter in the same world");
            if (activeImmediateReadersByWorld[index] != 0)
                Fail("ImmediateWriter overlapped an ImmediateReader in the same world");

            return activeInWorld;
        }

        void LeaveImmediateWriter(WorldRuntime& runtime)
        {
            {
                std::lock_guard lock{ immediateMutex };
                const size_t index = RuntimeIndexFor(runtime);
                if (index == kMissingRuntimeIndex ||
                    index >= activeImmediateWritersByWorld.size())
                {
                    Fail("ImmediateWriter missing runtime counter on leave");
                }
                else
                {
                    --activeImmediateWritersByWorld[index];
                }
            }

            immediateWriters.fetch_sub(1, std::memory_order_acq_rel);
        }

        [[noreturn]] void ThrowFailures() const
        {
            std::ostringstream oss;
            oss << "race stress recorded " << failures.load() << " failure(s)";
            for (const std::string& message : failureMessages)
                oss << "\n  - " << message;
            throw std::runtime_error(oss.str());
        }
    };

    StressState* g_stressState{ nullptr };

    void BusyJitter(uint32_t salt)
    {
        const uint32_t spins = 24 + (salt % 41);
        for (uint32_t i = 0; i < spins; ++i)
        {
            if ((i & 7u) == 0)
                std::this_thread::yield();
        }
    }

    template<typename T>
    constexpr std::string_view TypeName() noexcept
    {
#if defined(_MSC_VER)
        return __FUNCSIG__;
#else
        return __PRETTY_FUNCTION__;
#endif
    }

    uint64_t ParseUnsigned(std::string_view text, uint64_t fallback)
    {
        if (text.empty())
            return fallback;

        char* end = nullptr;
        const std::string copy{ text };
        const unsigned long long value = std::strtoull(copy.c_str(), &end, 10);
        return (end != copy.c_str()) ? static_cast<uint64_t>(value) : fallback;
    }

    std::string_view ValueAfterPrefix(std::string_view arg, std::string_view prefix)
    {
        if (arg.rfind(prefix, 0) != 0)
            return {};
        return arg.substr(prefix.size());
    }

    std::string EnvValue(const char* name)
    {
        char* rawValue = nullptr;
        size_t size = 0;
        if (_dupenv_s(&rawValue, &size, name) != 0 || rawValue == nullptr)
            return {};

        std::string value{ rawValue };
        std::free(rawValue);
        return value;
    }

    bool EnvEnabled(const char* name)
    {
        const std::string value = EnvValue(name);
        return !value.empty() && value[0] != '0';
    }

    uint64_t EnvUnsigned(const char* name, uint64_t fallback)
    {
        const std::string value = EnvValue(name);
        return !value.empty() ? ParseUnsigned(value, fallback) : fallback;
    }

    StressOptions ParseOptions(int argc, char** argv)
    {
        StressOptions options{};
        options.frames = EnvUnsigned("WITH_RACE_STRESS_FRAMES", options.frames);
        options.workers = static_cast<uint32_t>(
            EnvUnsigned("WITH_RACE_STRESS_WORKERS", options.workers));
        options.worlds = static_cast<uint32_t>(
            EnvUnsigned("WITH_RACE_STRESS_WORLDS", options.worlds));
        options.infinite = EnvEnabled("WITH_RACE_STRESS_INFINITE");
        options.verbose = EnvEnabled("WITH_RACE_STRESS_VERBOSE");
        options.strict = EnvEnabled("WITH_RACE_STRESS_STRICT");

        for (int i = 1; i < argc; ++i)
        {
            const std::string_view arg{ argv[i] };
            if (arg == "--race-stress-infinite")
                options.infinite = true;
            else if (arg == "--race-stress-verbose")
                options.verbose = true;
            else if (arg == "--race-stress-strict")
                options.strict = true;
            else if (const auto value = ValueAfterPrefix(arg, "--race-stress-frames="); !value.empty())
                options.frames = ParseUnsigned(value, options.frames);
            else if (const auto value = ValueAfterPrefix(arg, "--race-stress-workers="); !value.empty())
                options.workers = static_cast<uint32_t>(ParseUnsigned(value, options.workers));
            else if (const auto value = ValueAfterPrefix(arg, "--race-stress-worlds="); !value.empty())
                options.worlds = static_cast<uint32_t>(ParseUnsigned(value, options.worlds));
        }

        if (options.workers == 0)
            options.workers = std::max(4u, std::thread::hardware_concurrency());
        if (options.worlds == 0)
            options.worlds = 1;

        return options;
    }

    WorldDef MakeRaceStressWorldDef(WorldExecutionModelKey modelKey)
    {
        WorldDef def{};
        def.id = WorldDefId::Plaza;
        def.name = "TaskExecutorRaceStress";
        def.topology = { WorldKind::Dungeon, WorldInstanceType::Instanced };
        def.entryPolicy = {
            CreationPolicy::CreateOnDemand,
            JoinPolicy::FreeJoin,
            64,
            true,
            false,
            std::nullopt,
            std::nullopt
        };
        def.map.resourceId = 0;
        def.map.defaultPlayerSpawnPointId = SpawnPointIds::None;
        def.spawn = { SpawnSetId::None, std::nullopt };
        def.progressRule = {
            WorldClearConditionType::None,
            WorldFailConditionType::None,
            WorldCompletionActionType::None,
            std::nullopt,
            false
        };
        def.executionModelKey = modelKey;
        return def;
    }

    ExecutionOps MakeDummyValidOps()
    {
        return ExecutionOps(
            reinterpret_cast<WorldManager*>(0x1),
            reinterpret_cast<WorldRegistry*>(0x1),
            reinterpret_cast<WorldTransferService*>(0x1),
            reinterpret_cast<WorldAdmissionService*>(0x1),
            reinterpret_cast<NetIdRegistry*>(0x1),
            reinterpret_cast<PresenceManager*>(0x1));
    }

    struct SnapshotReaderBase
    {
        // Gap 2: WorldRuntime* 를 받아 per-world 병렬성 추적을 함께 수행한다.
        // Enter/Leave 사이의 BusyJitter 구간이 동시 실행 관측 창(window)이다.
        static void Run(StressState& state, WorldRuntime& runtime, int id)
        {
            state.EnterSnapshotReader(runtime);
            BusyJitter(static_cast<uint32_t>(id * 17 + state.currentFrame.load()));
            state.LeaveSnapshotReader(runtime);
        }
    };

    template<int Id>
    class SnapshotReaderSystem final : public System
    {
    public:
        using Self = SnapshotReaderSystem<Id>;

        static inline auto kMeta = MakeMetaStorage(
            SysTag<Self>(),
            TypeName<Self>(),
            std::array{ ReadSnapshot(ComponentRes<SnapshotProbeComp>()) });

        explicit SnapshotReaderSystem(StressState* state) : _state(state) {}

        const ExecMeta& Meta() const override { return kMeta.meta; }

        void Execute(SystemContext& ctx) override
        {
            const RuntimeTarget* targets = _state->TargetsFor(ctx.runtime);
            const Entity target =
                targets != nullptr ? targets->commonTarget : Entity::Null();
            const SnapshotProbeComp* component =
                !target.IsNull() ? ctx.ecs.GetComponent<SnapshotProbeComp>(target) : nullptr;
            const uint64_t frame =
                _state->currentFrame.load(std::memory_order_acquire);

            if (component == nullptr)
            {
                _state->Fail("SnapshotReader missing SnapshotProbeComp");
            }
            else if (component->value >= frame)
            {
                _state->Fail("SnapshotReader observed a deferred write before commit");
            }

            // Gap 2: runtime을 전달해 per-world 병렬성을 추적한다.
            SnapshotReaderBase::Run(*_state, ctx.runtime, Id);
        }

    private:
        StressState* _state{ nullptr };
    };

    template<int Id>
    class ImmediateReaderSystem final : public System
    {
    public:
        using Self = ImmediateReaderSystem<Id>;

        static inline auto kMeta = MakeMetaStorage(
            SysTag<Self>(),
            TypeName<Self>(),
            std::array{ ReadImmediate(ComponentRes<ImmediateProbeComp>()) });

        explicit ImmediateReaderSystem(StressState* state) : _state(state) {}

        const ExecMeta& Meta() const override { return kMeta.meta; }

        void Execute(SystemContext& ctx) override
        {
            _state->EnterImmediateReader(ctx.runtime);

            const RuntimeTarget* targets = _state->TargetsFor(ctx.runtime);
            const Entity target =
                targets != nullptr ? targets->commonTarget : Entity::Null();
            const ImmediateProbeComp* component =
                !target.IsNull() ? ctx.ecs.GetComponent<ImmediateProbeComp>(target) : nullptr;

            // Gap 3: hardcoded `frame * 100 + (kImmediateWriterCount - 1)` 대신
            // ImmediateWriter들이 실제로 마지막에 쓴 값을 per-world tracker에서 읽는다.
            // graph 보장에 의해 이 reader는 모든 writer가 완료된 이후에 실행되므로
            // tracker의 값은 최종 writer의 값이어야 한다.
            const size_t worldIdx = _state->RuntimeIndexFor(ctx.runtime);
            uint64_t expected = 0;
            if (worldIdx != StressState::kMissingRuntimeIndex &&
                worldIdx < _state->lastImmediateValueByWorld.size())
            {
                expected = std::atomic_ref<uint64_t>(
                    _state->lastImmediateValueByWorld[worldIdx])
                    .load(std::memory_order_acquire);
            }

            // expected == 0 이면 아직 어떤 writer도 실행되지 않은 것 → graph 오류.
            if (expected == 0)
            {
                _state->Fail("ImmediateReader ran before any ImmediateWriter in this world");
            }
            else if (component == nullptr)
            {
                _state->Fail("ImmediateReader missing ImmediateProbeComp");
            }
            else if (component->value != expected)
            {
                std::ostringstream oss;
                oss << "ImmediateReader observed value=" << component->value
                    << ", expected=" << expected
                    << " (from lastImmediateValueByWorld[" << worldIdx << "])";
                _state->Fail(oss.str());
            }

            BusyJitter(static_cast<uint32_t>(Id * 23 + _state->currentFrame.load()));

            _state->LeaveImmediateReader(ctx.runtime);
        }

    private:
        StressState* _state{ nullptr };
    };

    template<int Id>
    class ImmediateWriterSystem final : public System
    {
    public:
        using Self = ImmediateWriterSystem<Id>;

        static inline auto kMeta = MakeMetaStorage(
            SysTag<Self>(),
            TypeName<Self>(),
            std::array{ WriteImmediate(ComponentRes<ImmediateProbeComp>()) });

        explicit ImmediateWriterSystem(StressState* state) : _state(state) {}

        const ExecMeta& Meta() const override { return kMeta.meta; }

        void Execute(SystemContext& ctx) override
        {
            _state->EnterImmediateWriter(ctx.runtime);

            const RuntimeTarget* targets = _state->TargetsFor(ctx.runtime);
            const Entity target =
                targets != nullptr ? targets->commonTarget : Entity::Null();
            ImmediateProbeComp* component =
                !target.IsNull() ? ctx.ecs.GetMutableComponent<ImmediateProbeComp>(target) : nullptr;

            const uint64_t frame = _state->currentFrame.load(std::memory_order_acquire);
            const uint64_t writtenValue = frame * 100ull + static_cast<uint64_t>(Id);

            if (component == nullptr)
            {
                _state->Fail("ImmediateWriter missing ImmediateProbeComp");
            }
            else
            {
                component->value = writtenValue;
            }

            // Gap 3: 실제로 쓴 값을 per-world tracker에 기록한다.
            // ImmediateWriters는 graph dependency에 의해 직렬화되므로,
            // 마지막으로 실행된 writer의 값이 readers가 봐야 할 기대값이 된다.
            // 이 store는 LeaveImmediateWriter 직전에 수행되며,
            // readers는 모든 writers 이후에 실행됨이 graph에 의해 보장된다.
            const size_t worldIdx = _state->RuntimeIndexFor(ctx.runtime);
            if (worldIdx != StressState::kMissingRuntimeIndex &&
                worldIdx < _state->lastImmediateValueByWorld.size())
            {
                std::atomic_ref<uint64_t>(
                    _state->lastImmediateValueByWorld[worldIdx])
                    .store(writtenValue, std::memory_order_release);
            }

            BusyJitter(static_cast<uint32_t>(Id * 29 + _state->currentFrame.load()));

            _state->LeaveImmediateWriter(ctx.runtime);
        }

    private:
        StressState* _state{ nullptr };
    };

    template<int Id>
    class SnapshotDeferredWriterSystem final : public System
    {
    public:
        using Self = SnapshotDeferredWriterSystem<Id>;

        static inline auto kMeta = MakeMetaStorage(
            SysTag<Self>(),
            TypeName<Self>(),
            std::array{ WriteDeferred(ComponentRes<SnapshotProbeComp>()) });

        explicit SnapshotDeferredWriterSystem(StressState* state) : _state(state) {}

        const ExecMeta& Meta() const override { return kMeta.meta; }

        void Execute(SystemContext& ctx) override
        {
            const int active =
                _state->activeMutationRecorders.fetch_add(1, std::memory_order_acq_rel) + 1;
            _state->UpdateMax(_state->maxMutationRecorders, active);
            _state->recorded.fetch_add(1, std::memory_order_acq_rel);

            const RuntimeTarget* targets = _state->TargetsFor(ctx.runtime);
            const Entity target =
                targets != nullptr ? targets->commonTarget : Entity::Null();
            if (target.IsNull())
            {
                _state->Fail("SnapshotDeferredWriter missing bootstrap target");
            }
            else
            {
                SnapshotProbeComp component{};
                component.value = _state->currentFrame.load(std::memory_order_relaxed);
                ctx.runtime.DeferredUpsertComponent<SnapshotProbeComp>(
                    target,
                    component);
            }

            BusyJitter(static_cast<uint32_t>(Id * 41 + _state->currentFrame.load()));
            _state->activeMutationRecorders.fetch_sub(1, std::memory_order_acq_rel);
        }

    private:
        StressState* _state{ nullptr };
    };

    template<int Id>
    class ParallelDeferredRecorderSystem final : public System
    {
    public:
        using Self = ParallelDeferredRecorderSystem<Id>;

        static inline auto kMeta = MakeMetaStorage(
            SysTag<Self>(),
            TypeName<Self>(),
            std::array{ WriteDeferred(ComponentRes<DeferredProbeComp<Id>>()) });

        explicit ParallelDeferredRecorderSystem(StressState* state) : _state(state) {}

        const ExecMeta& Meta() const override { return kMeta.meta; }

        void Execute(SystemContext& ctx) override
        {
            const int active =
                _state->activeMutationRecorders.fetch_add(1, std::memory_order_acq_rel) + 1;
            _state->UpdateMax(_state->maxMutationRecorders, active);
            _state->recorded.fetch_add(1, std::memory_order_acq_rel);

            const RuntimeTarget* targets = _state->TargetsFor(ctx.runtime);
            const Entity target =
                targets != nullptr ? targets->deferredTargets[Id] : Entity::Null();
            if (target.IsNull())
            {
                _state->Fail("ParallelDeferredRecorder missing bootstrap target");
            }
            else
            {
                DeferredProbeComp<Id> component{};
                component.value = _state->currentFrame.load(std::memory_order_relaxed);
                ctx.runtime.DeferredUpsertComponent<DeferredProbeComp<Id>>(
                    target,
                    component);
            }

            BusyJitter(static_cast<uint32_t>(Id * 31 + _state->currentFrame.load()));
            _state->activeMutationRecorders.fetch_sub(1, std::memory_order_acq_rel);
        }

    private:
        StressState* _state{ nullptr };
    };

    template<int Id>
    class SerializedCommandBufferRecorderSystem final : public System
    {
    public:
        using Self = SerializedCommandBufferRecorderSystem<Id>;

        static inline auto kMeta = MakeMetaStorage(
            SysTag<Self>(),
            TypeName<Self>(),
            std::array{ WriteDeferred(CommandBufferRes()) });

        explicit SerializedCommandBufferRecorderSystem(StressState* state) : _state(state) {}

        const ExecMeta& Meta() const override { return kMeta.meta; }

        void Execute(SystemContext& ctx) override
        {
            const int commandBufferActiveInWorld =
                _state->EnterCommandBufferRecorder(ctx.runtime);
            if (commandBufferActiveInWorld > 1)
                _state->Fail("CommandBufferRes writers overlapped despite deferred write conflict");

            const int mutationActive =
                _state->activeMutationRecorders.fetch_add(1, std::memory_order_acq_rel) + 1;
            _state->UpdateMax(_state->maxMutationRecorders, mutationActive);
            _state->recorded.fetch_add(1, std::memory_order_acq_rel);

            const RuntimeTarget* targets = _state->TargetsFor(ctx.runtime);
            const Entity target =
                targets != nullptr ? targets->commandBufferTargets[Id] : Entity::Null();
            if (target.IsNull())
            {
                _state->Fail("SerializedCommandBufferRecorder missing bootstrap target");
            }
            else
            {
                CommandBufferProbeComp<Id> component{};
                component.value = _state->currentFrame.load(std::memory_order_relaxed);
                ctx.runtime.DeferredUpsertComponent<CommandBufferProbeComp<Id>>(
                    target,
                    component);
            }

            BusyJitter(static_cast<uint32_t>(Id * 37 + _state->currentFrame.load()));

            _state->activeMutationRecorders.fetch_sub(1, std::memory_order_acq_rel);
            _state->LeaveCommandBufferRecorder(ctx.runtime);
        }

    private:
        StressState* _state{ nullptr };
    };

    ExecCallResult CommitProbe(NodeExecContext& ctx)
    {
        (void)ctx;
        StressState& state = *g_stressState;
        state.commitProbeCount.fetch_add(1, std::memory_order_acq_rel);

        const uint64_t expectedRecorded =
            state.frameStartRecorded.load(std::memory_order_acquire) +
            state.expectedRecordsPerFrame;
        const uint64_t actualRecorded = state.recorded.load(std::memory_order_acquire);
        if (actualRecorded != expectedRecorded)
        {
            std::ostringstream oss;
            oss << "CommitProbe saw recorded=" << actualRecorded
                << ", expected=" << expectedRecorded;
            state.Fail(oss.str());
        }

        const uint64_t actualApplied = state.applied.load(std::memory_order_acquire);
        const uint64_t expectedApplied = state.frameStartApplied.load(std::memory_order_acquire);
        if (actualApplied != expectedApplied)
            state.Fail("CommitProbe observed deferred commands before commit scope flush");

        return ExecCallResult::Success;
    }

    ExecCallResult LifecycleProbe(NodeExecContext& ctx)
    {
        (void)ctx;
        StressState& state = *g_stressState;
        state.lifecycleProbeCount.fetch_add(1, std::memory_order_acq_rel);

        const uint64_t actualApplied = state.applied.load(std::memory_order_acquire);
        const uint64_t expectedApplied = state.frameStartApplied.load(std::memory_order_acquire);
        if (actualApplied != expectedApplied)
            state.Fail("LifecycleProbe observed lifecycle outbox before lifecycle flush");

        return ExecCallResult::Success;
    }

    template<int Id>
    void CheckDeferredProbe(
        WorldRuntime& runtime,
        const RuntimeTarget& targets,
        uint64_t expectedFrame,
        StressState& state)
    {
        ECSView view = runtime.MakeView();
        const DeferredProbeComp<Id>* component =
            view.GetComponent<DeferredProbeComp<Id>>(targets.deferredTargets[Id]);
        if (component == nullptr || component->value != expectedFrame)
        {
            std::ostringstream oss;
            oss << "DeferredProbeComp<" << Id << "> value mismatch";
            state.Fail(oss.str());
        }
    }

    template<int... Ids>
    void CheckDeferredProbes(
        WorldRuntime& runtime,
        const RuntimeTarget& targets,
        uint64_t expectedFrame,
        StressState& state,
        std::integer_sequence<int, Ids...>)
    {
        (CheckDeferredProbe<Ids>(runtime, targets, expectedFrame, state), ...);
    }

    template<int Id>
    void CheckCommandBufferProbe(
        WorldRuntime& runtime,
        const RuntimeTarget& targets,
        uint64_t expectedFrame,
        StressState& state)
    {
        ECSView view = runtime.MakeView();
        const CommandBufferProbeComp<Id>* component =
            view.GetComponent<CommandBufferProbeComp<Id>>(targets.commandBufferTargets[Id]);
        if (component == nullptr || component->value != expectedFrame)
        {
            std::ostringstream oss;
            oss << "CommandBufferProbeComp<" << Id << "> value mismatch";
            state.Fail(oss.str());
        }
    }

    template<int... Ids>
    void CheckCommandBufferProbes(
        WorldRuntime& runtime,
        const RuntimeTarget& targets,
        uint64_t expectedFrame,
        StressState& state,
        std::integer_sequence<int, Ids...>)
    {
        (CheckCommandBufferProbe<Ids>(runtime, targets, expectedFrame, state), ...);
    }

    void CheckCommonProbes(
        WorldRuntime& runtime,
        const RuntimeTarget& targets,
        uint64_t expectedFrame,
        StressState& state)
    {
        ECSView view = runtime.MakeView();
        const SnapshotProbeComp* snapshot =
            view.GetComponent<SnapshotProbeComp>(targets.commonTarget);
        if (snapshot == nullptr || snapshot->value != expectedFrame)
        {
            std::ostringstream oss;
            oss << "SnapshotProbeComp value mismatch";
            state.Fail(oss.str());
        }

        const ImmediateProbeComp* immediate =
            view.GetComponent<ImmediateProbeComp>(targets.commonTarget);
        const uint64_t expectedImmediate =
            expectedFrame * 100ull +
            static_cast<uint64_t>(kImmediateWriterCount - 1);
        if (immediate == nullptr || immediate->value != expectedImmediate)
        {
            std::ostringstream oss;
            oss << "ImmediateProbeComp value mismatch";
            state.Fail(oss.str());
        }
    }

    ExecCallResult ReconcileProbe(NodeExecContext& ctx)
    {
        WorldRuntime* runtime = ctx.TryGetRuntime();
        StressState& state = *g_stressState;
        state.reconcileProbeCount.fetch_add(1, std::memory_order_acq_rel);

        if (runtime == nullptr ||
            runtime->GetLifecycleFlushState() != WorldRuntimeLifecycleFlushState::Flushed)
        {
            state.Fail("ReconcileProbe ran before lifecycle flush completed");
            return ExecCallResult::Success;
        }

        const RuntimeTarget* targets = state.TargetsFor(*runtime);
        if (targets == nullptr)
        {
            state.Fail("ReconcileProbe missing runtime target set");
        }
        else
        {
            const uint64_t expectedFrame =
                state.currentFrame.load(std::memory_order_acquire);
            CheckCommonProbes(
                *runtime,
                *targets,
                expectedFrame,
                state);
            CheckDeferredProbes(
                *runtime,
                *targets,
                expectedFrame,
                state,
                std::make_integer_sequence<int, kDeferredRecorderCount>{});
            CheckCommandBufferProbes(
                *runtime,
                *targets,
                expectedFrame,
                state,
                std::make_integer_sequence<int, kCommandBufferRecorderCount>{});
            state.applied.fetch_add(
                static_cast<uint64_t>(
                    kSnapshotDeferredWriterCount +
                    kDeferredRecorderCount +
                    kCommandBufferRecorderCount),
                std::memory_order_acq_rel);
        }

        return ExecCallResult::Success;
    }

    template<int... Ids>
    void RegisterSnapshotReaders(
        SystemManager& manager,
        StressState& state,
        std::integer_sequence<int, Ids...>)
    {
        (manager.RegisterSystem<SnapshotReaderSystem<Ids>>(SystemPhase::Graph, &state), ...);
    }

    template<int... Ids>
    void RegisterImmediateReaders(
        SystemManager& manager,
        StressState& state,
        std::integer_sequence<int, Ids...>)
    {
        (manager.RegisterSystem<ImmediateReaderSystem<Ids>>(SystemPhase::Graph, &state), ...);
    }

    template<int... Ids>
    void RegisterImmediateWriters(
        SystemManager& manager,
        StressState& state,
        std::integer_sequence<int, Ids...>)
    {
        (manager.RegisterSystem<ImmediateWriterSystem<Ids>>(SystemPhase::Graph, &state), ...);
    }

    template<int... Ids>
    void RegisterSnapshotDeferredWriters(
        SystemManager& manager,
        StressState& state,
        std::integer_sequence<int, Ids...>)
    {
        (manager.RegisterSystem<SnapshotDeferredWriterSystem<Ids>>(SystemPhase::Graph, &state), ...);
    }

    template<int... Ids>
    void RegisterParallelDeferredRecorders(
        SystemManager& manager,
        StressState& state,
        std::integer_sequence<int, Ids...>)
    {
        (manager.RegisterSystem<ParallelDeferredRecorderSystem<Ids>>(SystemPhase::Graph, &state), ...);
    }

    template<int... Ids>
    void RegisterSerializedCommandBufferRecorders(
        SystemManager& manager,
        StressState& state,
        std::integer_sequence<int, Ids...>)
    {
        (manager.RegisterSystem<SerializedCommandBufferRecorderSystem<Ids>>(SystemPhase::Graph, &state), ...);
    }

    void RegisterStressSystems(SystemManager& manager, StressState& state)
    {
        RegisterSnapshotReaders(
            manager,
            state,
            std::make_integer_sequence<int, kSnapshotReaderCount>{});
        RegisterImmediateWriters(
            manager,
            state,
            std::make_integer_sequence<int, kImmediateWriterCount>{});
        RegisterImmediateReaders(
            manager,
            state,
            std::make_integer_sequence<int, kImmediateReaderCount>{});
        RegisterSnapshotDeferredWriters(
            manager,
            state,
            std::make_integer_sequence<int, kSnapshotDeferredWriterCount>{});
        RegisterParallelDeferredRecorders(
            manager,
            state,
            std::make_integer_sequence<int, kDeferredRecorderCount>{});
        RegisterSerializedCommandBufferRecorders(
            manager,
            state,
            std::make_integer_sequence<int, kCommandBufferRecorderCount>{});
    }

    template<int... Ids>
    void RegisterDeferredStorages(
        WorldRuntime& runtime,
        std::integer_sequence<int, Ids...>)
    {
        (runtime.RegisterStorage<DeferredProbeComp<Ids>>(), ...);
    }

    template<int... Ids>
    void RegisterCommandBufferStorages(
        WorldRuntime& runtime,
        std::integer_sequence<int, Ids...>)
    {
        (runtime.RegisterStorage<CommandBufferProbeComp<Ids>>(), ...);
    }

    void RegisterStressStorages(WorldRuntime& runtime)
    {
        runtime.RegisterStorage<SnapshotProbeComp>();
        runtime.RegisterStorage<ImmediateProbeComp>();
        RegisterDeferredStorages(
            runtime,
            std::make_integer_sequence<int, kDeferredRecorderCount>{});
        RegisterCommandBufferStorages(
            runtime,
            std::make_integer_sequence<int, kCommandBufferRecorderCount>{});
        runtime.FixStorages();
    }

    void RegisterManualSource(
        ExecutionSourceRegistry& sourceRegistry,
        WorldExecutionModel& model,
        ExecPhase phase,
        ExecLane lane,
        ExecNodeKind kind,
        ExecFn fn,
        const char* debugName)
    {
        ExecutionSourceDesc desc{};
        desc.token = sourceRegistry.AllocateToken();
        desc.phase = phase;
        desc.lane = lane;
        desc.kind = kind;
        desc.flags = ExecNodeFlag_None;
        desc.fn = fn;
        desc.debugName = debugName;

        if (!sourceRegistry.Register(desc))
            throw std::runtime_error("failed to register manual stress source");

        switch (phase)
        {
        case ExecPhase::Commit:
            model.commitSources.push_back(desc.token);
            break;
        case ExecPhase::LifecycleFlush:
            model.lifecycleFlushSources.push_back(desc.token);
            break;
        case ExecPhase::Reconcile:
            model.reconcileSources.push_back(desc.token);
            break;
        default:
            throw std::runtime_error("unsupported manual stress source phase");
        }
    }

    template<int Id>
    void BootstrapDeferredTarget(
        WorldRuntime& runtime,
        uint64_t frameIndex,
        RuntimeTarget& targets)
    {
        const Entity entity = runtime.ReserveEntity();
        if (entity.IsNull())
            throw std::runtime_error("bootstrap deferred ReserveEntity failed");

        DeferredProbeComp<Id> component{};
        component.value = frameIndex;
        runtime.DeferredUpsertComponent<DeferredProbeComp<Id>>(entity, component);
        targets.deferredTargets[Id] = entity;
    }

    template<int... Ids>
    void BootstrapDeferredTargets(
        WorldRuntime& runtime,
        uint64_t frameIndex,
        RuntimeTarget& targets,
        std::integer_sequence<int, Ids...>)
    {
        (BootstrapDeferredTarget<Ids>(runtime, frameIndex, targets), ...);
    }

    template<int Id>
    void BootstrapCommandBufferTarget(
        WorldRuntime& runtime,
        uint64_t frameIndex,
        RuntimeTarget& targets)
    {
        const Entity entity = runtime.ReserveEntity();
        if (entity.IsNull())
            throw std::runtime_error("bootstrap command-buffer ReserveEntity failed");

        CommandBufferProbeComp<Id> component{};
        component.value = frameIndex;
        runtime.DeferredUpsertComponent<CommandBufferProbeComp<Id>>(entity, component);
        targets.commandBufferTargets[Id] = entity;
    }

    template<int... Ids>
    void BootstrapCommandBufferTargets(
        WorldRuntime& runtime,
        uint64_t frameIndex,
        RuntimeTarget& targets,
        std::integer_sequence<int, Ids...>)
    {
        (BootstrapCommandBufferTarget<Ids>(runtime, frameIndex, targets), ...);
    }

    RuntimeTarget BootstrapTargetEntities(WorldRuntime& runtime, uint64_t frameIndex)
    {
        if (!runtime.BeginFrame(frameIndex, 0.0, 0.016))
            throw std::runtime_error("bootstrap BeginFrame failed");

        RuntimeTarget targets{};
        targets.runtime = &runtime;

        const Entity common = runtime.ReserveEntity();
        if (common.IsNull())
            throw std::runtime_error("bootstrap common ReserveEntity failed");
        targets.commonTarget = common;

        SnapshotProbeComp snapshot{};
        snapshot.value = frameIndex;
        runtime.DeferredUpsertComponent<SnapshotProbeComp>(common, snapshot);

        ImmediateProbeComp immediate{};
        immediate.value = frameIndex;
        runtime.DeferredUpsertComponent<ImmediateProbeComp>(common, immediate);

        BootstrapDeferredTargets(
            runtime,
            frameIndex,
            targets,
            std::make_integer_sequence<int, kDeferredRecorderCount>{});
        BootstrapCommandBufferTargets(
            runtime,
            frameIndex,
            targets,
            std::make_integer_sequence<int, kCommandBufferRecorderCount>{});

        if (!runtime.FlushFrameCommands())
            throw std::runtime_error("bootstrap FlushFrameCommands failed");
        if (!runtime.FlushLifecycleCommands())
            throw std::runtime_error("bootstrap FlushLifecycleCommands failed");

        runtime.ClearLifecycleOutbox();
        return targets;
    }

    void ValidateBuildResult(const BuildResult& buildResult)
    {
        if (!buildResult.success || buildResult.HasError())
        {
            std::ostringstream oss;
            oss << "race stress graph build failed";
            for (const BuildDiagnostic& diagnostic : buildResult.diagnostics)
                oss << "\n  - " << diagnostic.message;
            throw std::runtime_error(oss.str());
        }
    }
}

void RunTaskExecutorRaceStressTest(int argc, char** argv)
{
    const StressOptions options = ParseOptions(argc, argv);

    StressState state{};
    g_stressState = &state;
    state.expectedRecordsPerFrame =
        static_cast<uint64_t>(options.worlds) *
        static_cast<uint64_t>(
            kSnapshotDeferredWriterCount +
            kDeferredRecorderCount +
            kCommandBufferRecorderCount);

    WorldExecutionModel model{};
    model.key = kRaceStressModelKey;
    WorldDef def = MakeRaceStressWorldDef(kRaceStressModelKey);

    std::vector<std::unique_ptr<WorldRuntime>> runtimes;
    runtimes.reserve(options.worlds);

    for (uint32_t i = 0; i < options.worlds; ++i)
    {
        auto runtime = std::make_unique<WorldRuntime>(WorldRuntimeCreateParams{
            &def,
            &model,
            nullptr,
            nullptr
        });

        RegisterStressStorages(*runtime);
        RegisterStressSystems(runtime->GetSystemManager(), state);
        runtimes.push_back(std::move(runtime));
    }

    ExecutionSourceRegistry sourceRegistry{};
    AutoSystemBridge bridge{};
    {
        AutoSystemBridge::BridgeResult result =
            bridge.RegisterSources(
                SystemPhase::Graph,
                ExecPhase::Simulate,
                runtimes.front()->GetSystemManager(),
                sourceRegistry);
        if (!result.success)
            throw std::runtime_error(result.errorMessage);

        result = bridge.BuildModelFromRegisteredSources(
            SystemPhase::Graph,
            ExecPhase::Simulate,
            runtimes.front()->GetSystemManager(),
            sourceRegistry,
            model);
        if (!result.success)
            throw std::runtime_error(result.errorMessage);
    }

    RegisterManualSource(
        sourceRegistry,
        model,
        ExecPhase::Commit,
        ExecLane::Serial,
        ExecNodeKind::PostCommitFinalize,
        &CommitProbe,
        "RaceStress.CommitProbe");
    RegisterManualSource(
        sourceRegistry,
        model,
        ExecPhase::LifecycleFlush,
        ExecLane::Serial,
        ExecNodeKind::LifecycleFlush,
        &LifecycleProbe,
        "RaceStress.LifecycleProbe");
    RegisterManualSource(
        sourceRegistry,
        model,
        ExecPhase::Reconcile,
        ExecLane::Serial,
        ExecNodeKind::Reconcile,
        &ReconcileProbe,
        "RaceStress.ReconcileProbe");

    WorldExecutionModelRegistry modelRegistry{};
    if (!modelRegistry.Register(model, sourceRegistry))
        throw std::runtime_error("WorldExecutionModelRegistry rejected race stress model");

    for (std::unique_ptr<WorldRuntime>& runtime : runtimes)
    {
        AutoSystemBridge::BridgeResult result =
            bridge.BindRuntimeDispatch(
                SystemPhase::Graph,
                ExecPhase::Simulate,
                runtime->GetSystemManager(),
                sourceRegistry,
                *runtime);
        if (!result.success)
            throw std::runtime_error(result.errorMessage);

        if (!runtime->Initialize())
            throw std::runtime_error("race stress runtime initialize failed");

        state.targets.push_back(BootstrapTargetEntities(*runtime, 1));
    }
    state.activeCommandBufferRecordersByWorld.assign(state.targets.size(), 0);
    state.activeImmediateReadersByWorld.assign(state.targets.size(), 0);
    state.activeImmediateWritersByWorld.assign(state.targets.size(), 0);
    // Gap 2: per-world snapshot reader 카운터 초기화
    state.activeSnapshotReadersByWorld.assign(state.targets.size(), 0);
    // Gap 3: per-world last immediate value 초기화 (0 = 아직 미기록 sentinel)
    state.lastImmediateValueByWorld.assign(state.targets.size(), 0);

    WorldFrameSelectionSet selections{};
    std::vector<WorldRuntime*> runtimeByScope;
    std::vector<WorldId> worldIdByScope;
    runtimeByScope.reserve(runtimes.size());
    worldIdByScope.reserve(runtimes.size());

    for (uint32_t i = 0; i < options.worlds; ++i)
    {
        const ExecScopeId scopeId = static_cast<ExecScopeId>(i);
        const WorldId worldId = WorldId::Create(static_cast<uint32_t>(100 + i), 1);
        selections.selections.push_back(WorldFrameSelection{
            scopeId,
            worldId.GetRaw(),
            kRaceStressModelKey
        });
        runtimeByScope.push_back(runtimes[i].get());
        worldIdByScope.push_back(worldId);
    }

    ExecutionGraphBuilder graphBuilder{};
    ExecutionGraphBuildPolicy buildPolicy{};
    FrameBuildContext buildContext{};
    buildContext.frameSelectionSet = &selections;
    buildContext.executionModelRegistry = &modelRegistry;
    buildContext.executionSourceRegistry = &sourceRegistry;
    buildContext.buildPolicy = &buildPolicy;

    BuildResult buildResult = graphBuilder.Build(buildContext);
    ValidateBuildResult(buildResult);
    if (buildResult.graph.edges.empty())
        throw std::runtime_error("race stress graph unexpectedly has no dependency edges");

    // Gap 4: 등록된 시스템 구성에 근거한 최소 edge 수를 검증한다.
    //
    // 그래프 빌더는 conflict를 bipartite(전체 쌍) 대신 chain 방식으로 생성한다.
    // 이 때문에 WriteImmediate writers 간 직렬화는 chain 상 마지막 writer에서만
    // reader들로의 edge가 만들어진다(앞쪽 writer→reader는 전이적으로 보장).
    //
    // 실측(worlds=4 → actual=40) 기준 per-world 구조:
    //   WriteImmediate chain : IW[0]→IW[1]               : 1 edge
    //   Last IW → all IR     : IW[1]→IR[0..5]            : 6 edges
    //   CommandBuffer chain  : CB[0]→CB[1]→CB[2]→CB[3]  : 3 edges
    //   ---------------------------------------------------
    //   per-world 실측값                                  : 10 edges
    //
    // kMinEdgesPerWorld는 실측값보다 낮게 설정해 그래프 빌더 내부 정책의
    // 소폭 변화(chain 깊이 변경 등)에 대한 내성을 확보한다.
    // 그러나 0에 가까운 edge 수(conflict 누락)는 반드시 감지해야 한다.
    //
    // edges pool 항목 수 = 2 × logical directed edges
    // (각 edge가 predecessor의 succList 와 successor의 predList 에 각각 1회 기록됨)
    {
        constexpr size_t kMinEdgesPerWorld = 8;   // 실측 10의 80% — 정책 변화 내성
        const size_t minExpectedEdges = kMinEdgesPerWorld * options.worlds;
        const size_t actualEdges = buildResult.graph.edges.size() / 2;
        if (actualEdges < minExpectedEdges)
        {
            std::ostringstream oss;
            oss << "race stress graph has too few dependency edges"
                << " (actual=" << actualEdges
                << ", minExpected=" << minExpectedEdges
                << ", worlds=" << options.worlds << ")";
            throw std::runtime_error(oss.str());
        }
    }

    std::vector<ExecNodeRuntime> nodeRuntime(buildResult.graph.nodes.size());
    std::vector<ExecScopeRuntime> scopeRuntime(buildResult.graph.scopeCount);

    ExecutionOps ops = MakeDummyValidOps();
    FrameExecContext frameExec{};
    frameExec.graph = &buildResult.graph;
    frameExec.ops = &ops;
    frameExec.runtimeByScope = runtimeByScope;
    frameExec.worldIdByScope = worldIdByScope;

    ExecRuntimeState execRuntime{};
    execRuntime.BindViews(
        std::span<ExecNodeRuntime>(nodeRuntime.data(), nodeRuntime.size()),
        std::span<ExecScopeRuntime>(scopeRuntime.data(), scopeRuntime.size()),
        buildResult.graph.simulateNodeCount);

    TaskExecutor executor(options.workers);
    if (!executor.IsInitialized())
        throw std::runtime_error("TaskExecutor did not initialize for race stress");

    std::cout
        << "[RACE_STRESS] begin"
        << " frames=" << (options.infinite ? 0 : options.frames)
        << " infinite=" << (options.infinite ? 1 : 0)
        << " workers=" << options.workers
        << " worlds=" << options.worlds
        << " strict=" << (options.strict ? 1 : 0)
        << " nodes=" << buildResult.graph.nodes.size()
        << " simulateNodes=" << buildResult.graph.simulateNodeCount
        << " dependencyEdges=" << (buildResult.graph.edges.size() / 2)
        << " edgePoolEntries=" << buildResult.graph.edges.size()
        << "\n";

    const auto started = std::chrono::steady_clock::now();
    uint64_t frame = 0;
    for (; options.infinite || frame < options.frames; ++frame)
    {
        const uint64_t frameIndex = frame + 2;
        state.currentFrame.store(frameIndex, std::memory_order_release);
        state.frameStartRecorded.store(state.recorded.load(std::memory_order_acquire), std::memory_order_release);
        state.frameStartApplied.store(state.applied.load(std::memory_order_acquire), std::memory_order_release);

        // Gap 3: 프레임 시작 시 per-world last immediate value를 0(미기록 sentinel)으로 리셋한다.
        // 이전 프레임에서 기록된 값이 다음 프레임 ImmediateReader 검증에 오염되는 것을 방지한다.
        for (uint64_t& v : state.lastImmediateValueByWorld)
            std::atomic_ref<uint64_t>(v).store(0, std::memory_order_release);

        for (std::unique_ptr<WorldRuntime>& runtime : runtimes)
        {
            if (!runtime->BeginFrame(frameIndex, static_cast<double>(frameIndex) * 0.016, 0.016))
                throw std::runtime_error("race stress BeginFrame failed");
        }

        if (!executor.ExecuteFrame(frameExec, execRuntime, sourceRegistry))
            throw std::runtime_error("race stress ExecuteFrame returned false");

        // SPEC-EXEC-QUEUE-001 §개선방향 §6: strict mode — invariant counter 확인.
        // 하나라도 0이 아니면 즉시 실패 처리한다.
        if (options.strict)
        {
            const TaskExecutorFrameDiagnostics& diag = executor.GetLastFrameDiagnostics();
            const TaskExecutorInvariantCounters& inv = diag.invariants;
            if (inv.AnyNonZero())
            {
                std::ostringstream oss;
                oss << "[STRICT] frame=" << (frame + 1)
                    << " invariant counter(s) non-zero:"
                    << " staleQueuedEntryDiscarded=" << inv.staleQueuedEntryDiscarded
                    << " successorReadyTransitionSkipped=" << inv.successorReadyTransitionSkipped
                    << " successorCancelTransitionSkipped=" << inv.successorCancelTransitionSkipped
                    << " remainingDepsUnderflowAttempt=" << inv.remainingDepsUnderflowAttempt
                    << " dispatchReadyToQueuedFailed=" << inv.dispatchReadyToQueuedFailed
                    << " duplicateCompletionAttempt=" << inv.duplicateCompletionAttempt
                    << " scopeRemainingUnderflowAttempt=" << inv.scopeRemainingUnderflowAttempt
                    << " simulateRemainingUnderflowAttempt=" << inv.simulateRemainingUnderflowAttempt;
                throw std::runtime_error(oss.str());
            }
        }

        if (state.failures.load(std::memory_order_acquire) != 0)
            state.ThrowFailures();

        const uint64_t expectedRecorded =
            state.frameStartRecorded.load(std::memory_order_acquire) +
            state.expectedRecordsPerFrame;
        const uint64_t expectedApplied =
            state.frameStartApplied.load(std::memory_order_acquire) +
            state.expectedRecordsPerFrame;
        if (state.recorded.load(std::memory_order_acquire) != expectedRecorded ||
            state.applied.load(std::memory_order_acquire) != expectedApplied)
        {
            state.Fail("frame total recorded/applied mismatch after ExecuteFrame");
            state.ThrowFailures();
        }

        for (std::unique_ptr<WorldRuntime>& runtime : runtimes)
        {
            if (runtime->IsFaulted())
                throw std::runtime_error(runtime->GetFault().message);
            runtime->ClearLifecycleOutbox();
        }

        if (options.verbose && ((frame + 1) % 1000 == 0))
        {
            std::cout
                << "[RACE_STRESS] frame=" << (frame + 1)
                << " recorded=" << state.recorded.load()
                << " applied=" << state.applied.load()
                << " maxSnapshotReaders=" << state.maxSnapshotReaders.load()
                << " maxMutationRecorders=" << state.maxMutationRecorders.load()
                << "\n";
        }
    }

    const auto elapsed = std::chrono::steady_clock::now() - started;
    const double elapsedSec =
        std::chrono::duration_cast<std::chrono::duration<double>>(elapsed).count();

    if (state.maxSnapshotReaders.load(std::memory_order_acquire) < 2)
        throw std::runtime_error("race stress did not observe snapshot reader parallelism (global)");
    // Gap 2: global counter는 서로 다른 world reader 2개로도 충족되므로
    // per-world 병렬성을 별도로 확인한다.
    if (state.maxSnapshotReadersPerWorld < 2)
        throw std::runtime_error("race stress did not observe same-world snapshot reader parallelism");
    if (state.maxMutationRecorders.load(std::memory_order_acquire) < 2)
        throw std::runtime_error("race stress did not observe concurrent deferred command recording");
    if (state.maxCommandBufferRecordersPerWorld != 1)
        throw std::runtime_error("CommandBufferRes writers were not serialized within the same world");
    // Gap 1: worlds >= 2 일 때 cross-world CommandBuffer 병렬성이 실제로 관측됐는지 확인한다.
    // 같은 scope 안에서는 1개로 직렬화되지만, 서로 다른 scope 간에는 충돌이 없으므로
    // global 최대 동시 실행 수는 world 수만큼 도달 가능해야 한다.
    if (options.worlds >= 2 &&
        state.maxCommandBufferRecorders.load(std::memory_order_acquire) < 2)
    {
        throw std::runtime_error(
            "race stress did not observe cross-world CommandBuffer recorder parallelism "
            "(worlds >= 2 but maxCommandBufferRecorders < 2)");
    }
    if (state.maxImmediateReadersPerWorld < 2)
        throw std::runtime_error("race stress did not observe immediate read parallelism");

    // 마지막 프레임의 invariant 카운터 상태 출력 (soak mode에서 누적 파악용).
    {
        const TaskExecutorFrameDiagnostics& diag = executor.GetLastFrameDiagnostics();
        const TaskExecutorInvariantCounters& inv = diag.invariants;
        std::cout
            << "[PASS] TaskExecutor race stress"
            << " frames=" << frame
            << " worlds=" << options.worlds
            << " workers=" << options.workers
            << " strict=" << (options.strict ? 1 : 0)
            << " elapsedSec=" << elapsedSec
            << " recorded=" << state.recorded.load()
            << " applied=" << state.applied.load()
            << " maxSnapshotReadersGlobal=" << state.maxSnapshotReaders.load()
            << " maxSnapshotReadersPerWorld=" << state.maxSnapshotReadersPerWorld
            << " maxMutationRecorders=" << state.maxMutationRecorders.load()
            << " maxCommandBufferRecordersGlobal=" << state.maxCommandBufferRecorders.load()
            << " maxCommandBufferRecordersPerWorld=" << state.maxCommandBufferRecordersPerWorld
            << " maxImmediateReadersGlobal=" << state.maxImmediateReaders.load()
            << " maxImmediateReadersPerWorld=" << state.maxImmediateReadersPerWorld
            << " lastFrame_staleQueuedEntry=" << inv.staleQueuedEntryDiscarded
            << " lastFrame_successorReadySkipped=" << inv.successorReadyTransitionSkipped
            << " lastFrame_successorCancelSkipped=" << inv.successorCancelTransitionSkipped
            << " lastFrame_depsUnderflow=" << inv.remainingDepsUnderflowAttempt
            << " lastFrame_dispatchFailed=" << inv.dispatchReadyToQueuedFailed
            << " lastFrame_duplicateCompletion=" << inv.duplicateCompletionAttempt
            << " lastFrame_scopeUnderflow=" << inv.scopeRemainingUnderflowAttempt
            << " lastFrame_simUnderflow=" << inv.simulateRemainingUnderflowAttempt
            << "\n";
    }
}
