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
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// NOTE:
// This is a smoke-test scaffold for the new SystemScheduler + ThreadPool path.
// It assumes the following project APIs already exist and may need small renames:
// - class System with ctor System(WorldRuntime&, int priority)
// - struct SystemScheduleDesc { System* system; const SystemMeta* meta; uint32 registrationOrder; }
// - WorldRuntime can be constructed from ThreadPool& + IWorldImpl&
//
// The visible uploaded files still show WorldRuntime using JobGraph and
// SystemScheduler::Execute unfinished, so apply those runtime patches first.

namespace
{
    using namespace std::chrono_literals;

    struct TestReadComp {};
    struct TestEvent {};

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
                !maxParallelReads.compare_exchange_strong(
                    prev, value, std::memory_order_relaxed))
            {
            }
        }
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

        const SystemMeta& Meta() const override
        {
            return kMeta.meta;
        }

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

        const SystemMeta& Meta() const override
        {
            return kMeta.meta;
        }

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

        const SystemMeta& Meta() const override
        {
            return kMeta.meta;
        }

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

        const SystemMeta& Meta() const override
        {
            return kMeta.meta;
        }

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

        const SystemMeta& Meta() const override
        {
            return kMeta.meta;
        }

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

        const SystemMeta& Meta() const override
        {
            return kMeta.meta;
        }

        void Execute(const double dT) override
        {
            _state.afterFailureRan.store(true, std::memory_order_release);
            _state.PushTrace("AfterFail");
        }

    private:
        SharedState& _state;
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
}

void RunSystemSchedulerSmokeTest()
{
    ThreadPool pool(4);
    DummyWorldImpl impl;
    WorldRuntime rt(pool, impl);

    SharedState state;
    SharedState failState;

    ReadA1System read1(rt, state);
    ReadA2System read2(rt, state);
    EmitSystem emit(rt, state);
    ConsumeSystem consume(rt, state);

    FailSystem fail(rt);
    AfterFailSystem after(rt, failState);

    std::array<System*, 4> okSystems
    {
        &read1,
        &read2,
        &emit,
        &consume
    };

    std::array<System*, 2> failSystems
    {
        &fail,
        &after
    };

    SystemScheduler scheduler;

    const auto okDescs = BuildScheduleDescs(okSystems);
    const auto okSchedule = scheduler.Compile(okDescs);

    const auto failDescs = BuildScheduleDescs(failSystems);
    const auto failSchedule = scheduler.Compile(failDescs);

    scheduler.Execute(okSchedule, pool, 0.016);

    assert(state.maxParallelReads.load(std::memory_order_acquire) >= 2);
    assert(state.emitDone.load(std::memory_order_acquire));
    assert(state.consumeObserved.load(std::memory_order_acquire));

    bool threw = false;
    try
    {
        scheduler.Execute(failSchedule, pool, 0.016);
    }
    catch (const std::exception&)
    {
        threw = true;
    }

    assert(threw);
    assert(!failState.afterFailureRan.load(std::memory_order_acquire));

    std::cout << "maxParallelReads = " << state.maxParallelReads.load() << '\n';
    std::cout << "emitDone = " << state.emitDone.load() << '\n';
    std::cout << "consumeObserved = " << state.consumeObserved.load() << '\n';
    std::cout << "threw = " << threw << '\n';
    std::cout << "afterFailureRan = " << failState.afterFailureRan.load() << '\n';
}

void main()
{
    RunSystemSchedulerSmokeTest();
}