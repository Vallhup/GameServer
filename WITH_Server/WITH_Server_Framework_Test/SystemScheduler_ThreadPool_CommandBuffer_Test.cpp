#include "pch.h"

#include "WorldRuntime.h"
#include "World.h"
#include "System.h"
#include "SystemScheduler.h"
#include "SystemMetaStorage.h"
#include "SystemMetaHelper.h"
#include "ThreadPool.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <exception>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <array>

namespace
{
    using namespace std::chrono_literals;

    struct TestReadComp {};
    struct TestEvent {};

    struct CommandProbeA {};
    struct CommandProbeB {};
    struct SerializeProbe {};
    struct RecordThenFailProbe {};

    struct TestRunner
    {
        int passed{ 0 };
        int failed{ 0 };

        void Section(const std::string& name)
        {
            std::cout << "\n=== " << name << " ===\n";
        }

        void Check(bool cond, const std::string& name)
        {
            if (cond)
            {
                ++passed;
                std::cout << "[PASS] " << name << '\n';
            }
            else
            {
                ++failed;
                std::cout << "[FAIL] " << name << '\n';
            }
        }

        void Summary() const
        {
            std::cout << "\n=== Summary ===\n";
            std::cout << "passed = " << passed << '\n';
            std::cout << "failed = " << failed << '\n';
        }
    };

    struct SharedState
    {
        std::atomic<int> runningReads{ 0 };
        std::atomic<int> maxParallelReads{ 0 };

        std::atomic<bool> emitDone{ false };
        std::atomic<bool> consumeObserved{ false };
        std::atomic<bool> afterFailureRan{ false };

        std::mutex traceMtx;
        std::vector<std::string> trace;

        void PushTrace(const std::string& s)
        {
            std::lock_guard lock(traceMtx);
            trace.push_back(s);
        }

        void UpdateMaxParallel(int value)
        {
            int prev = maxParallelReads.load(std::memory_order_relaxed);
            while (prev < value &&
                !maxParallelReads.compare_exchange_weak(
                    prev, value, std::memory_order_relaxed))
            {
            }
        }
    };

    struct CommandState
    {
        std::atomic<int> runningRecorders{ 0 };
        std::atomic<int> maxParallelRecorders{ 0 };

        std::atomic<int> recorded{ 0 };
        std::atomic<int> applied{ 0 };
        std::atomic<int> payloadSum{ 0 };

        std::atomic<bool> observerRan{ false };
        std::atomic<bool> observerSawInvisible{ false };

        std::mutex traceMtx;
        std::vector<std::string> trace;

        void PushTrace(const std::string& s)
        {
            std::lock_guard lock(traceMtx);
            trace.push_back(s);
        }

        void UpdateMaxParallel(int value)
        {
            int prev = maxParallelRecorders.load(std::memory_order_relaxed);
            while (prev < value &&
                !maxParallelRecorders.compare_exchange_weak(
                    prev, value, std::memory_order_relaxed))
            {
            }
        }
    };

    struct SerializeState
    {
        std::atomic<int> running{ 0 };
        std::atomic<int> maxParallel{ 0 };
        std::atomic<int> applied{ 0 };
        std::atomic<int> payloadSum{ 0 };

        std::mutex traceMtx;
        std::vector<std::string> trace;

        void PushTrace(const std::string& s)
        {
            std::lock_guard lock(traceMtx);
            trace.push_back(s);
        }

        void UpdateMaxParallel(int value)
        {
            int prev = maxParallel.load(std::memory_order_relaxed);
            while (prev < value &&
                !maxParallel.compare_exchange_weak(
                    prev, value, std::memory_order_relaxed))
            {
            }
        }
    };

    struct LeakState
    {
        std::atomic<int> recorded{ 0 };
        std::atomic<int> applied{ 0 };
    };

    class DummyWorldImpl final : public IWorldImpl
    {
    public:
        void SpawnInitial(WorldRuntime& rt) override {}
        Entity SpawnPlayer(WorldRuntime& rt, uint32 connId) override { return Entity{}; }
        void Build(WorldRuntime& rt) override {}
    };

    class ReadA1System final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<ReadA1System>(),
            "ReadA1System",
            std::array{ ReadSnapshot(ComponentRes<TestReadComp>()) }
        );

        ReadA1System(WorldRuntime& rt, SharedState& state)
            : System(rt), _state(state)
        {
        }

        const SystemMeta& Meta() const override { return kMeta.meta; }

        void Execute(const double dT) override
        {
            const int now = _state.runningReads.fetch_add(1, std::memory_order_acq_rel) + 1;
            _state.UpdateMaxParallel(now);
            _state.PushTrace("ReadA1 begin");

            std::this_thread::sleep_for(100ms);

            _state.PushTrace("ReadA1 end");
            _state.runningReads.fetch_sub(1, std::memory_order_acq_rel);
        }

    private:
        SharedState& _state;
    };

    class ReadA2System final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<ReadA2System>(),
            "ReadA2System",
            std::array{ ReadSnapshot(ComponentRes<TestReadComp>()) }
        );

        ReadA2System(WorldRuntime& rt, SharedState& state)
            : System(rt), _state(state)
        {
        }

        const SystemMeta& Meta() const override { return kMeta.meta; }

        void Execute(const double dT) override
        {
            const int now = _state.runningReads.fetch_add(1, std::memory_order_acq_rel) + 1;
            _state.UpdateMaxParallel(now);
            _state.PushTrace("ReadA2 begin");

            std::this_thread::sleep_for(100ms);

            _state.PushTrace("ReadA2 end");
            _state.runningReads.fetch_sub(1, std::memory_order_acq_rel);
        }

    private:
        SharedState& _state;
    };

    class EmitSystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<EmitSystem>(),
            "EmitSystem",
            std::array{ EmitDeferred(EventRes<TestEvent>()) }
        );

        EmitSystem(WorldRuntime& rt, SharedState& state)
            : System(rt), _state(state)
        {
        }

        const SystemMeta& Meta() const override { return kMeta.meta; }

        void Execute(const double dT) override
        {
            _state.emitDone.store(true, std::memory_order_release);
            _state.PushTrace("Emit");
        }

    private:
        SharedState& _state;
    };

    class ConsumeSystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<ConsumeSystem>(),
            "ConsumeSystem",
            std::array{ ConsumeCommit(EventRes<TestEvent>()) }
        );

        ConsumeSystem(WorldRuntime& rt, SharedState& state)
            : System(rt), _state(state)
        {
        }

        const SystemMeta& Meta() const override { return kMeta.meta; }

        void Execute(const double dT) override
        {
            if (!_state.emitDone.load(std::memory_order_acquire))
                throw std::runtime_error("ConsumeSystem ran before EmitSystem.");

            _state.consumeObserved.store(true, std::memory_order_release);
            _state.PushTrace("Consume");
        }

    private:
        SharedState& _state;
    };

    class FailSystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<FailSystem>(),
            "FailSystem",
            std::array{ ReadSnapshot(ComponentRes<TestReadComp>()) },
            std::array{ SysTag<class AfterFailSystem>() }
        );

        FailSystem(WorldRuntime& rt)
            : System(rt)
        {
        }

        const SystemMeta& Meta() const override { return kMeta.meta; }

        void Execute(const double dT) override
        {
            throw std::runtime_error("Expected failure from FailSystem.");
        }
    };

    class AfterFailSystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<AfterFailSystem>(),
            "AfterFailSystem",
            std::array{ ReadSnapshot(ComponentRes<TestReadComp>()) }
        );

        AfterFailSystem(WorldRuntime& rt, SharedState& state)
            : System(rt), _state(state)
        {
        }

        const SystemMeta& Meta() const override { return kMeta.meta; }

        void Execute(const double dT) override
        {
            _state.afterFailureRan.store(true, std::memory_order_release);
            _state.PushTrace("AfterFail");
        }

    private:
        SharedState& _state;
    };

    class RecordCommandASystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<RecordCommandASystem>(),
            "RecordCommandASystem",
            std::array{
                ReadSnapshot(ComponentRes<TestReadComp>()),
                WriteDeferred(ExternalRes<CommandProbeA>())
            }
        );

        RecordCommandASystem(WorldRuntime& rt, CommandState& state)
            : System(rt), _state(state)
        {
        }

        const SystemMeta& Meta() const override { return kMeta.meta; }

        void Execute(const double dT) override
        {
            const int now = _state.runningRecorders.fetch_add(1, std::memory_order_acq_rel) + 1;
            _state.UpdateMaxParallel(now);

            _state.recorded.fetch_add(1, std::memory_order_acq_rel);
            _state.PushTrace("RecordA begin");

            std::this_thread::sleep_for(80ms);

            _runtime.Commands().Enqueue(
                [st = &_state](WorldRuntime& rt)
                {
                    st->applied.fetch_add(1, std::memory_order_acq_rel);
                    st->payloadSum.fetch_add(10, std::memory_order_acq_rel);
                    st->PushTrace("RecordA apply");
                });

            _state.PushTrace("RecordA end");
            _state.runningRecorders.fetch_sub(1, std::memory_order_acq_rel);
        }

    private:
        CommandState& _state;
    };

    class RecordCommandBSystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<RecordCommandBSystem>(),
            "RecordCommandBSystem",
            std::array{
                ReadSnapshot(ComponentRes<TestReadComp>()),
                WriteDeferred(ExternalRes<CommandProbeB>())
            }
        );

        RecordCommandBSystem(WorldRuntime& rt, CommandState& state)
            : System(rt), _state(state)
        {
        }

        const SystemMeta& Meta() const override { return kMeta.meta; }

        void Execute(const double dT) override
        {
            const int now = _state.runningRecorders.fetch_add(1, std::memory_order_acq_rel) + 1;
            _state.UpdateMaxParallel(now);

            _state.recorded.fetch_add(1, std::memory_order_acq_rel);
            _state.PushTrace("RecordB begin");

            std::this_thread::sleep_for(80ms);

            _runtime.Commands().Enqueue(
                [st = &_state](WorldRuntime& rt)
                {
                    st->applied.fetch_add(1, std::memory_order_acq_rel);
                    st->payloadSum.fetch_add(20, std::memory_order_acq_rel);
                    st->PushTrace("RecordB apply");
                });

            _state.PushTrace("RecordB end");
            _state.runningRecorders.fetch_sub(1, std::memory_order_acq_rel);
        }

    private:
        CommandState& _state;
    };

    class ObserveBeforeFlushSystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<ObserveBeforeFlushSystem>(),
            "ObserveBeforeFlushSystem",
            std::array{
                ReadSnapshot(ExternalRes<CommandProbeA>()),
                ReadSnapshot(ExternalRes<CommandProbeB>())
            },
            std::array<SystemTag, 0>{},
            std::array{
                SysTag<RecordCommandASystem>(),
                SysTag<RecordCommandBSystem>()
            }
        );

        ObserveBeforeFlushSystem(WorldRuntime& rt, CommandState& state)
            : System(rt), _state(state)
        {
        }

        const SystemMeta& Meta() const override { return kMeta.meta; }

        void Execute(const double dT) override
        {
            _state.observerRan.store(true, std::memory_order_release);

            const bool invisible =
                (_state.applied.load(std::memory_order_acquire) == 0) &&
                (_state.payloadSum.load(std::memory_order_acquire) == 0);

            _state.observerSawInvisible.store(invisible, std::memory_order_release);
            _state.PushTrace("ObserveBeforeFlush");
        }

    private:
        CommandState& _state;
    };

    class SerializeWrite1System final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<SerializeWrite1System>(),
            "SerializeWrite1System",
            std::array{
                WriteDeferred(ExternalRes<SerializeProbe>())
            },
            std::array{ SysTag<class SerializeWrite2System>() }
        );

        SerializeWrite1System(WorldRuntime& rt, SerializeState& state)
            : System(rt), _state(state)
        {
        }

        const SystemMeta& Meta() const override { return kMeta.meta; }

        void Execute(const double dT) override
        {
            const int now = _state.running.fetch_add(1, std::memory_order_acq_rel) + 1;
            _state.UpdateMaxParallel(now);
            _state.PushTrace("Serialize1 begin");

            std::this_thread::sleep_for(60ms);

            _runtime.Commands().Enqueue(
                [st = &_state](WorldRuntime& rt)
                {
                    st->applied.fetch_add(1, std::memory_order_acq_rel);
                    st->payloadSum.fetch_add(1, std::memory_order_acq_rel);
                    st->PushTrace("Serialize1 apply");
                });

            _state.PushTrace("Serialize1 end");
            _state.running.fetch_sub(1, std::memory_order_acq_rel);
        }

    private:
        SerializeState& _state;
    };

    class SerializeWrite2System final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<SerializeWrite2System>(),
            "SerializeWrite2System",
            std::array{
                WriteDeferred(ExternalRes<SerializeProbe>())
            }
        );

        SerializeWrite2System(WorldRuntime& rt, SerializeState& state)
            : System(rt), _state(state)
        {
        }

        const SystemMeta& Meta() const override { return kMeta.meta; }

        void Execute(const double dT) override
        {
            const int now = _state.running.fetch_add(1, std::memory_order_acq_rel) + 1;
            _state.UpdateMaxParallel(now);
            _state.PushTrace("Serialize2 begin");

            std::this_thread::sleep_for(60ms);

            _runtime.Commands().Enqueue(
                [st = &_state](WorldRuntime& rt)
                {
                    st->applied.fetch_add(1, std::memory_order_acq_rel);
                    st->payloadSum.fetch_add(2, std::memory_order_acq_rel);
                    st->PushTrace("Serialize2 apply");
                });

            _state.PushTrace("Serialize2 end");
            _state.running.fetch_sub(1, std::memory_order_acq_rel);
        }

    private:
        SerializeState& _state;
    };

    class RecordThenFailSystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<RecordThenFailSystem>(),
            "RecordThenFailSystem",
            std::array{
                WriteDeferred(ExternalRes<RecordThenFailProbe>())
            }
        );

        RecordThenFailSystem(WorldRuntime& rt, LeakState& state)
            : System(rt), _state(state)
        {
        }

        const SystemMeta& Meta() const override { return kMeta.meta; }

        void Execute(const double dT) override
        {
            _state.recorded.fetch_add(1, std::memory_order_acq_rel);

            _runtime.Commands().Enqueue(
                [st = &_state](WorldRuntime& rt)
                {
                    st->applied.fetch_add(1, std::memory_order_acq_rel);
                });

            throw std::runtime_error("RecordThenFailSystem failed after recording a command.");
        }

    private:
        LeakState& _state;
    };

    template <size_t N>
    std::vector<SystemScheduleDesc> BuildScheduleDescs(const std::array<System*, N>& systems)
    {
        std::vector<SystemScheduleDesc> descs;
        descs.reserve(N);

        for (size_t i = 0; i < N; ++i)
        {
            System* sys = systems[i];
            assert(sys != nullptr);

            descs.push_back(SystemScheduleDesc{
                .system = sys,
                .meta = &sys->Meta(),
                .registrationOrder = static_cast<int>(i)
                });
        }

        return descs;
    }

    void TestParallelReadAndEmitConsume(TestRunner& tr)
    {
        tr.Section("Parallel Read + Emit/Consume");

        ThreadPool pool(4);
        DummyWorldImpl impl;
        WorldRuntime rt(pool, impl);

        SharedState state;

        ReadA1System read1(rt, state);
        ReadA2System read2(rt, state);
        EmitSystem emit(rt, state);
        ConsumeSystem consume(rt, state);

        std::array<System*, 4> systems
        {
            &read1,
            &read2,
            &emit,
            &consume
        };

        SystemScheduler scheduler;
        const auto descs = BuildScheduleDescs(systems);
        const auto schedule = scheduler.Compile(descs);

        scheduler.Execute(schedule, pool, 0.016);

        tr.Check(state.maxParallelReads.load(std::memory_order_acquire) >= 2,
            "snapshot reads run in parallel");
        tr.Check(state.emitDone.load(std::memory_order_acquire),
            "emit system ran");
        tr.Check(state.consumeObserved.load(std::memory_order_acquire),
            "consume system observed emit after dependency ordering");
    }

    void TestFailureStopsDependentSystem(TestRunner& tr)
    {
        tr.Section("Failure Propagation");

        ThreadPool pool(4);
        DummyWorldImpl impl;
        WorldRuntime rt(pool, impl);

        SharedState state;

        FailSystem fail(rt);
        AfterFailSystem after(rt, state);

        std::array<System*, 2> systems
        {
            &fail,
            &after
        };

        SystemScheduler scheduler;
        const auto descs = BuildScheduleDescs(systems);
        const auto schedule = scheduler.Compile(descs);

        bool threw = false;
        try
        {
            scheduler.Execute(schedule, pool, 0.016);
        }
        catch (const std::exception&)
        {
            threw = true;
        }

        tr.Check(threw, "batch exception is rethrown");
        tr.Check(!state.afterFailureRan.load(std::memory_order_acquire),
            "dependent system does not run after failure");
    }

    void TestCommandRecordVisibilityAndFlush(TestRunner& tr)
    {
        tr.Section("CommandBuffer Record / Visibility / Flush");

        ThreadPool pool(4);
        DummyWorldImpl impl;
        WorldRuntime rt(pool, impl);

        CommandState state;

        RecordCommandASystem recordA(rt, state);
        RecordCommandBSystem recordB(rt, state);
        ObserveBeforeFlushSystem observe(rt, state);

        std::array<System*, 3> systems
        {
            &recordA,
            &recordB,
            &observe
        };

        SystemScheduler scheduler;
        const auto descs = BuildScheduleDescs(systems);
        const auto schedule = scheduler.Compile(descs);

        scheduler.Execute(schedule, pool, 0.016);

        tr.Check(state.maxParallelRecorders.load(std::memory_order_acquire) >= 2,
            "command recorders can run in parallel");
        tr.Check(state.recorded.load(std::memory_order_acquire) == 2,
            "both commands are recorded");
        tr.Check(state.observerRan.load(std::memory_order_acquire),
            "pre-flush observer ran");
        tr.Check(state.observerSawInvisible.load(std::memory_order_acquire),
            "commands are invisible before flush");
        tr.Check(state.applied.load(std::memory_order_acquire) == 0,
            "no command applied before flush");
        tr.Check(state.payloadSum.load(std::memory_order_acquire) == 0,
            "payload sum stays zero before flush");

        rt.Commands().Commit(rt);

        tr.Check(state.applied.load(std::memory_order_acquire) == 2,
            "flush applies exactly two commands");
        tr.Check(state.payloadSum.load(std::memory_order_acquire) == 30,
            "flush applies expected payload sum");

        rt.Commands().Commit(rt);

        tr.Check(state.applied.load(std::memory_order_acquire) == 2,
            "second flush does not reapply commands");
        tr.Check(state.payloadSum.load(std::memory_order_acquire) == 30,
            "second flush keeps payload sum unchanged");
    }

    void TestConflictingDeferredWritesSerialize(TestRunner& tr)
    {
        tr.Section("Deferred Write Serialization");

        ThreadPool pool(4);
        DummyWorldImpl impl;
        WorldRuntime rt(pool, impl);

        SerializeState state;

        SerializeWrite1System s1(rt, state);
        SerializeWrite2System s2(rt, state);

        std::array<System*, 2> systems
        {
            &s1,
            &s2
        };

        SystemScheduler scheduler;
        const auto descs = BuildScheduleDescs(systems);
        const auto schedule = scheduler.Compile(descs);

        scheduler.Execute(schedule, pool, 0.016);

        tr.Check(state.maxParallel.load(std::memory_order_acquire) == 1,
            "same deferred resource writes are serialized");

        rt.Commands().Commit(rt);

        tr.Check(state.applied.load(std::memory_order_acquire) == 2,
            "serialized writers both flush");
        tr.Check(state.payloadSum.load(std::memory_order_acquire) == 3,
            "serialized writers preserve both payloads");
    }

    void TestFailureAfterRecordLeakPolicy(TestRunner& tr)
    {
        tr.Section("Failure After Record Leak Policy");

        ThreadPool pool(4);
        DummyWorldImpl impl;
        WorldRuntime rt(pool, impl);

        LeakState state;

        RecordThenFailSystem sys(rt, state);

        std::array<System*, 1> systems
        {
            &sys
        };

        SystemScheduler scheduler;
        const auto descs = BuildScheduleDescs(systems);
        const auto schedule = scheduler.Compile(descs);

        bool threw = false;
        try
        {
            try
            {
                scheduler.Execute(schedule, pool, 0.016);
                rt.Commands().Commit(rt);
            }
            catch (...)
            {
                rt.Commands().Clear();
                throw;
            }
        }
        catch (const std::exception&)
        {
            threw = true;
        }

        tr.Check(threw, "record-then-fail system throws");
        tr.Check(state.recorded.load(std::memory_order_acquire) == 1,
            "command was recorded before failure");
        tr.Check(state.applied.load(std::memory_order_acquire) == 0,
            "command is not applied during failing execute");

        // 이 검사는 의도적으로 엄격합니다.
        // 여기서 FAIL이 나오면 '실패한 프레임의 pending command가 나중 flush에 누수됨'을 뜻합니다.
        rt.Commands().Commit(rt);

        tr.Check(state.applied.load(std::memory_order_acquire) == 0,
            "failed frame must not leak stale pending commands");
    }

    void TestRuntimeRunAutoCommit(TestRunner& tr)
    {
        tr.Section("WorldRuntime::Run Auto Commit");

        ThreadPool pool(2);
        DummyWorldImpl impl;
        WorldRuntime rt(pool, impl);

        std::atomic<int> applied{ 0 };
        std::atomic<int> payload{ 0 };

        rt.GraphBuild();

        rt.Commands().Enqueue(
            [&applied, &payload](WorldRuntime& rt)
            {
                applied.fetch_add(1, std::memory_order_acq_rel);
                payload.fetch_add(77, std::memory_order_acq_rel);
            });

        tr.Check(applied.load(std::memory_order_acquire) == 0,
            "before Run, queued command is not applied");

        rt.Run(0.016);

        tr.Check(applied.load(std::memory_order_acquire) == 1,
            "Run auto-flushes command buffer");
        tr.Check(payload.load(std::memory_order_acquire) == 77,
            "Run applies queued payload once");

        rt.Run(0.016);

        tr.Check(applied.load(std::memory_order_acquire) == 1,
            "next Run does not replay stale commands");
        tr.Check(payload.load(std::memory_order_acquire) == 77,
            "next Run keeps payload unchanged");
    }

    struct RuntimeProbeA {};
    struct RuntimeProbeB {};
    struct RuntimeLeakProbe {};
    struct DummyComp {};

    struct RuntimeSuccessState
    {
        std::atomic<int> recorded{ 0 };
        std::atomic<int> applied{ 0 };
        std::atomic<int> payloadSum{ 0 };

        std::atomic<bool> postRan{ false };
        std::atomic<bool> postSawInvisible{ false };

        std::atomic<int> runningGraph{ 0 };
        std::atomic<int> maxParallelGraph{ 0 };

        void UpdateMaxParallel(int value)
        {
            int prev = maxParallelGraph.load(std::memory_order_relaxed);
            while (prev < value &&
                !maxParallelGraph.compare_exchange_weak(
                    prev, value, std::memory_order_relaxed))
            {
            }
        }
    };

    struct RuntimeFailState
    {
        std::atomic<int> recorded{ 0 };
        std::atomic<int> applied{ 0 };

        std::atomic<int> failExecCount{ 0 };
        std::atomic<bool> postRanOnFailFrame{ false };
        std::atomic<bool> postRanOnRecoveryFrame{ false };
    };

    class GraphRecordASystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<GraphRecordASystem>(),
            "GraphRecordASystem",
            std::array{
                ReadSnapshot(ComponentRes<DummyComp>()),
                WriteDeferred(ExternalRes<RuntimeProbeA>())
            }
        );

        GraphRecordASystem(WorldRuntime& rt, RuntimeSuccessState& state)
            : System(rt), _state(state)
        {
        }

        const SystemMeta& Meta() const override
        {
            return kMeta.meta;
        }

        void Execute(const double dT) override
        {
            const int now = _state.runningGraph.fetch_add(1, std::memory_order_acq_rel) + 1;
            _state.UpdateMaxParallel(now);

            _state.recorded.fetch_add(1, std::memory_order_acq_rel);
            std::this_thread::sleep_for(50ms);

            _runtime.Commands().Enqueue(
                [st = &_state](WorldRuntime& rt)
                {
                    st->applied.fetch_add(1, std::memory_order_acq_rel);
                    st->payloadSum.fetch_add(10, std::memory_order_acq_rel);
                });

            _state.runningGraph.fetch_sub(1, std::memory_order_acq_rel);
        }

    private:
        RuntimeSuccessState& _state;
    };

    class GraphRecordBSystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<GraphRecordBSystem>(),
            "GraphRecordBSystem",
            std::array{
                ReadSnapshot(ComponentRes<DummyComp>()),
                WriteDeferred(ExternalRes<RuntimeProbeB>())
            }
        );

        GraphRecordBSystem(WorldRuntime& rt, RuntimeSuccessState& state)
            : System(rt), _state(state)
        {
        }

        const SystemMeta& Meta() const override
        {
            return kMeta.meta;
        }

        void Execute(const double dT) override
        {
            const int now = _state.runningGraph.fetch_add(1, std::memory_order_acq_rel) + 1;
            _state.UpdateMaxParallel(now);

            _state.recorded.fetch_add(1, std::memory_order_acq_rel);
            std::this_thread::sleep_for(50ms);

            _runtime.Commands().Enqueue(
                [st = &_state](WorldRuntime& rt)
                {
                    st->applied.fetch_add(1, std::memory_order_acq_rel);
                    st->payloadSum.fetch_add(20, std::memory_order_acq_rel);
                });

            _state.runningGraph.fetch_sub(1, std::memory_order_acq_rel);
        }

    private:
        RuntimeSuccessState& _state;
    };

    class PostObserveBeforeCommitSystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<PostObserveBeforeCommitSystem>(),
            "PostObserveBeforeCommitSystem",
            std::array{
                ReadSnapshot(ExternalRes<RuntimeProbeA>()),
                ReadSnapshot(ExternalRes<RuntimeProbeB>())
            }
        );

        PostObserveBeforeCommitSystem(WorldRuntime& rt, RuntimeSuccessState& state)
            : System(rt), _state(state)
        {
        }

        const SystemMeta& Meta() const override
        {
            return kMeta.meta;
        }

        void Execute(const double dT) override
        {
            _state.postRan.store(true, std::memory_order_release);

            const bool invisible =
                (_state.applied.load(std::memory_order_acquire) == 0) &&
                (_state.payloadSum.load(std::memory_order_acquire) == 0);

            _state.postSawInvisible.store(invisible, std::memory_order_release);
        }

    private:
        RuntimeSuccessState& _state;
    };

    class FailOnceAfterRecordSystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<FailOnceAfterRecordSystem>(),
            "FailOnceAfterRecordSystem",
            std::array{
                WriteDeferred(ExternalRes<RuntimeLeakProbe>())
            }
        );

        FailOnceAfterRecordSystem(WorldRuntime& rt, RuntimeFailState& state)
            : System(rt), _state(state)
        {
        }

        const SystemMeta& Meta() const override
        {
            return kMeta.meta;
        }

        void Execute(const double dT) override
        {
            const int nth = _state.failExecCount.fetch_add(1, std::memory_order_acq_rel);

            if (nth == 0)
            {
                _state.recorded.fetch_add(1, std::memory_order_acq_rel);

                _runtime.Commands().Enqueue(
                    [st = &_state](WorldRuntime& rt)
                    {
                        st->applied.fetch_add(1, std::memory_order_acq_rel);
                    });

                throw std::runtime_error("Intentional fail after recording command.");
            }

            // recovery frame: do nothing
        }

    private:
        RuntimeFailState& _state;
    };

    class PostMarkerSystem final : public System
    {
    public:
        static inline auto kMeta = MakeMetaStorage(
            SysTag<PostMarkerSystem>(),
            "PostMarkerSystem",
            std::array<AccessSpec, 0>{}
        );

        PostMarkerSystem(WorldRuntime& rt, RuntimeFailState& state)
            : System(rt), _state(state)
        {
        }

        const SystemMeta& Meta() const override
        {
            return kMeta.meta;
        }

        void Execute(const double dT) override
        {
            if (_state.failExecCount.load(std::memory_order_acquire) <= 1)
                _state.postRanOnFailFrame.store(true, std::memory_order_release);
            else
                _state.postRanOnRecoveryFrame.store(true, std::memory_order_release);
        }

    private:
        RuntimeFailState& _state;
    };

    // 로컬 ECS 등록 API에 맞춰 이 한 줄만 바꾸면 됩니다.
    // 예: ecs.AddSystem<T>(SystemPhase::Graph, rt, args...)
    // 혹은 ecs.Systems().RegisterSystem<T>(...)
    template<typename T, typename... Args>
    T* RegisterGraph(WorldRuntime& rt, Args&&... args)
    {
        auto& ecs = rt.GetECS();
        return ecs.AddSystem<T>(SystemPhase::Graph, rt, std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    T* RegisterPost(WorldRuntime& rt, Args&&... args)
    {
        auto& ecs = rt.GetECS();
        return ecs.AddSystem<T>(SystemPhase::Post, rt, std::forward<Args>(args)...);
    }

    void TestWorldRuntimeRunSuccess(TestRunner& tr)
    {
        tr.Section("WorldRuntime::Run success path");

        ThreadPool pool(4);

        class DummyWorldImpl final : public IWorldImpl
        {
        public:
            void SpawnInitial(WorldRuntime& rt) override {}
            Entity SpawnPlayer(WorldRuntime& rt, uint32 connId) override { return Entity{}; }
            void Build(WorldRuntime& rt) override {}
        };

        DummyWorldImpl impl;
        WorldRuntime rt(pool, impl);

        RuntimeSuccessState state;

        RegisterGraph<GraphRecordASystem>(rt, state);
        RegisterGraph<GraphRecordBSystem>(rt, state);
        RegisterPost<PostObserveBeforeCommitSystem>(rt, state);

        rt.GraphBuild();
        rt.Run(0.016);

        tr.Check(state.recorded.load(std::memory_order_acquire) == 2,
            "success frame records two commands");
        tr.Check(state.postRan.load(std::memory_order_acquire),
            "post phase runs");
        tr.Check(state.postSawInvisible.load(std::memory_order_acquire),
            "post phase cannot observe deferred commands before commit");
        tr.Check(state.applied.load(std::memory_order_acquire) == 2,
            "run commits both recorded commands");
        tr.Check(state.payloadSum.load(std::memory_order_acquire) == 30,
            "run applies expected payload sum");
        tr.Check(state.maxParallelGraph.load(std::memory_order_acquire) >= 2,
            "graph systems run in parallel on success path");
    }

    void TestWorldRuntimeRunFailureClearsPending(TestRunner& tr)
    {
        tr.Section("WorldRuntime::Run failure clears pending");

        ThreadPool pool(4);

        class DummyWorldImpl final : public IWorldImpl
        {
        public:
            void SpawnInitial(WorldRuntime& rt) override {}
            Entity SpawnPlayer(WorldRuntime& rt, uint32 connId) override { return Entity{}; }
            void Build(WorldRuntime& rt) override {}
        };

        DummyWorldImpl impl;
        WorldRuntime rt(pool, impl);

        RuntimeFailState state;

        RegisterGraph<FailOnceAfterRecordSystem>(rt, state);
        RegisterPost<PostMarkerSystem>(rt, state);

        rt.GraphBuild();

        bool threw = false;
        try
        {
            rt.Run(0.016); // first frame: record + throw
        }
        catch (const std::exception&)
        {
            threw = true;
        }

        tr.Check(threw,
            "failing frame rethrows exception");
        tr.Check(state.recorded.load(std::memory_order_acquire) == 1,
            "failing frame records one deferred command before throw");
        tr.Check(state.applied.load(std::memory_order_acquire) == 0,
            "failing frame does not commit recorded command");
        tr.Check(!state.postRanOnFailFrame.load(std::memory_order_acquire),
            "post phase does not run on failing frame");

        // second frame: no new command, no throw
        rt.Run(0.016);

        tr.Check(state.applied.load(std::memory_order_acquire) == 0,
            "failed frame does not leak stale pending commands into next frame");
        tr.Check(state.postRanOnRecoveryFrame.load(std::memory_order_acquire),
            "recovery frame runs normally");
    }
}

void RunSystemSchedulerSmokeTest()
{
    TestRunner tr;

    TestParallelReadAndEmitConsume(tr);
    TestFailureStopsDependentSystem(tr);
    TestCommandRecordVisibilityAndFlush(tr);
    TestConflictingDeferredWritesSerialize(tr);
    TestFailureAfterRecordLeakPolicy(tr);
    TestRuntimeRunAutoCommit(tr);
    TestWorldRuntimeRunSuccess(tr);
    TestWorldRuntimeRunFailureClearsPending(tr);

    tr.Summary();
}

int main()
{
    RunSystemSchedulerSmokeTest();
    return 0;
}