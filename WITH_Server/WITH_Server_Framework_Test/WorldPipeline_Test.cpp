#include "pch.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <vector>
#include <string>

#include "World.h"
#include "WorldId.h"
#include "WorldDesc.h"
#include "WorldRuntime.h"
#include "System.h"
#include "ThreadPool.h"

enum class InboxKind : uint8_t { Start, Late };
struct InboxMsg { uint32 tick; InboxKind kind; };

class DoubleBufferedInbox {
public:
    void PushBack(const InboxMsg& m) {
        std::lock_guard<std::mutex> lock{ _mtx };
        _back.push_back(m);
    }

    // tick boundary에서 WorldThread가 호출 (Back->Front 확정)
    void Swap() {
        std::lock_guard<std::mutex> lock{ _mtx };
        _front.clear();
        _front.swap(_back);   // front = back, back = empty
    }

    void DrainFront(std::vector<InboxMsg>& out) {
        std::lock_guard<std::mutex> lock{ _mtx };
        out.insert(out.end(), _front.begin(), _front.end());
        _front.clear();
    }

private:
    std::mutex _mtx;
    std::vector<InboxMsg> _front;
    std::vector<InboxMsg> _back;
};

struct InputFrame {
    bool hasStart = false; uint32 startTick = 0;
    bool hasLate = false; uint32 lateTick = 0;
    void Clear() { hasStart = false; hasLate = false; }
};

struct Probe {
    uint32 tick = 0;
    struct Step { uint32 tick; const char* phase; };
    std::vector<Step> steps;
};

class InboxDrainFrontSystem final : public System {
public:
    InboxDrainFrontSystem(WorldRuntime& rt, DoubleBufferedInbox& inbox, InputFrame& frame, Probe& p, int prio = 0)
        : System(rt, prio), _inbox(inbox), _frame(frame), _p(p) {
    }

    void Execute(const double) override {
        _p.steps.push_back({ _p.tick, "Pre" });
        _frame.Clear();

        _tmp.clear();
        _inbox.DrainFront(_tmp);

        for (auto& m : _tmp) {
            if (m.kind == InboxKind::Start) { _frame.hasStart = true; _frame.startTick = m.tick; }
            if (m.kind == InboxKind::Late) { _frame.hasLate = true;  _frame.lateTick = m.tick; }
        }
    }

    std::vector<std::type_index> ReadResources() const override { return {}; }
    std::vector<std::type_index> WriteResources() const override { return {}; }

private:
    DoubleBufferedInbox& _inbox;
    InputFrame& _frame;
    Probe& _p;
    std::vector<InboxMsg> _tmp;
};

class GraphCheckSystem final : public System {
public:
    GraphCheckSystem(WorldRuntime& rt, InputFrame& frame, Probe& p, int prio = 0)
        : System(rt, prio), _frame(frame), _p(p) {
    }

    void Execute(const double) override {
        _p.steps.push_back({ _p.tick, "Graph" });

        // Start는 같은 tick에 반드시 보임
        assert(_frame.hasStart);
        assert(_frame.startTick == _p.tick);

        // Late는 "이전 tick" 것만 보임 (tick==0이면 없음)
        if (_p.tick == 0) {
            assert(!_frame.hasLate);
        }
        else {
            assert(_frame.hasLate);
            assert(_frame.lateTick == (_p.tick - 1));
        }
    }

    std::vector<std::type_index> ReadResources() const override { return {}; }
    std::vector<std::type_index> WriteResources() const override { return {}; }

private:
    InputFrame& _frame;
    Probe& _p;
};

class PostPushLateSystem final : public System {
public:
    PostPushLateSystem(WorldRuntime& rt, DoubleBufferedInbox& inbox, Probe& p, int prio = 0)
        : System(rt, prio), _inbox(inbox), _p(p) {
    }

    void Execute(const double) override {
        _p.steps.push_back({ _p.tick, "Post" });
        // 틱 도중 도착한 입력은 Back에만 들어가야 함(다음 tick으로 이월)
        _inbox.PushBack(InboxMsg{ _p.tick, InboxKind::Late });
    }

    std::vector<std::type_index> ReadResources() const override { return {}; }
    std::vector<std::type_index> WriteResources() const override { return {}; }

private:
    DoubleBufferedInbox& _inbox;
    Probe& _p;
};

class NetInboxSwapTestWorldImpl final : public IWorldImpl {
public:
    NetInboxSwapTestWorldImpl(DoubleBufferedInbox& inbox, InputFrame& frame, Probe& p)
        : _inbox(inbox), _frame(frame), _p(p) {
    }

    void SpawnInitial(WorldRuntime&) override {}
    Entity SpawnPlayer(WorldRuntime& rt, uint32) override { return rt.GetECS().CreateEntity(); }

    void Build(WorldRuntime& rt) override {
        ECS& ecs = rt.GetECS();
        ecs.AddSystem<InboxDrainFrontSystem>(SystemPhase::Pre, rt, _inbox, _frame, _p);
        ecs.AddSystem<GraphCheckSystem>(SystemPhase::Graph, rt, _frame, _p);
        ecs.AddSystem<PostPushLateSystem>(SystemPhase::Post, rt, _inbox, _p);
    }

private:
    DoubleBufferedInbox& _inbox;
    InputFrame& _frame;
    Probe& _p;
};

int main() {
    DoubleBufferedInbox inbox;
    InputFrame frame;
    Probe probe;

    ThreadPool pool(1);

    WorldDesc desc{};
    desc.worldType = 0; desc.worldRulesetId = 0; desc.Capacity = 0;
    desc.mapId = 0; desc.netPolicy = 0;

    auto impl = std::make_unique<NetInboxSwapTestWorldImpl>(inbox, frame, probe);
    World world(WorldId::Create(1, 1), desc, pool, std::move(impl));
    world.Init();

    constexpr double dt = 1.0 / 60.0;
    constexpr uint32 ticks = 10;

    for (uint32 t = 0; t < ticks; ++t) {
        probe.tick = t;

        // tick 시작 전에 도착한 입력은 Back에 넣고, boundary에서 Swap으로 확정
        inbox.PushBack(InboxMsg{ t, InboxKind::Start });
        inbox.Swap(); // <-- 이게 핵심(틱 경계)

        world.Update(dt);
    }

    std::cout << "[OK] Inbox swap boundary test passed.\n";
    return 0;
}