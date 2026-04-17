#include "TestWorldImpl.h"
#include "pch.h"
#include "System.h"
#include <cstdint>

struct WorldDebugState {
    uint64_t preTicks = 0;
    uint64_t graphTicks = 0;
    uint64_t postTicks = 0;
};

// 월드/런타임의 리소스에 WorldDebugState 같은 걸 넣어두고 참조한다고 가정
// 만약 ResourceRegistry가 아직 없으면, ECS에 임시로 public 멤버로 둬도 됩니다.
class DebugPreSystem final : public System {
public:
    explicit DebugPreSystem(WorldDebugState& s, ECS& e, int p = 0) : System(e, p), _s(s) {}
    void Execute(const float) override { ++_s.preTicks; }

    std::vector<std::type_index> ReadResources() const override
    {
        return {  };
    }

    std::vector<std::type_index> WriteResources() const override
    {
        return {  };
    }

private:
    WorldDebugState& _s;
};

class DebugGraphSystem final : public System {
public:
    explicit DebugGraphSystem(WorldDebugState& s, ECS& e, int p = 0) : System(e, p), _s(s) {}
    void Execute(const float) override { ++_s.graphTicks; }

    std::vector<std::type_index> ReadResources() const override
    {
        return {  };
    }

    std::vector<std::type_index> WriteResources() const override
    {
        return {  };
    }

private:
    WorldDebugState& _s;
};

class DebugPostSystem final : public System {
public:
    explicit DebugPostSystem(WorldDebugState& s, std::string name, ECS& e, int p = 0)
        : System(e, p), _s(s), _name(std::move(name)) {}
    void Execute(const float) override
    {
        ++_s.postTicks;
        // 출력은 너무 많이 하면 섞이니, 아주 드물게만 찍으십시오.
        if ((_s.postTicks % 300) == 0) { 
            std::cout << "[" << _name << "] pre=" << _s.preTicks
                << " graph=" << _s.graphTicks
                << " post=" << _s.postTicks << "\n";
        }
    }

    std::vector<std::type_index> ReadResources() const override
    {
        return {  };
    }

    std::vector<std::type_index> WriteResources() const override
    {
        return {  };
    }

private:
    WorldDebugState& _s;
    std::string _name;
};

#include "ECS.h"
#include "World.h"

class TestWorldImpl final : public IWorldImpl {
public:
    explicit TestWorldImpl(std::string name) : _name(std::move(name)) {}

    void Build(WorldRuntime& rt) override
    {
        // 월드별 상태
        // (ResourceRegistry가 있으면 거기에 넣는 게 맞고, 없으면 Impl 멤버 참조로도 충분)
        auto& ecs = rt.GetECS();

        // Pre/Graph/Post 시스템 등록
        ecs.AddSystem<DebugPreSystem>(SystemPhase::Pre, _state, ecs, 0);
        ecs.AddSystem<DebugGraphSystem>(SystemPhase::Graph, _state, ecs, 0);
        ecs.AddSystem<DebugPostSystem>(SystemPhase::Post, _state, _name, ecs, 0);

        // 수동 의존성 테스트도 하고 싶으면 Graph 시스템 2개를 더 만들고
        // rt.AddManualDependency<A, B>(); 넣으면 됩니다.
    }

    void Execute(WorldRuntime& rt, float dt) override
    {
        rt.Run(dt);
    }

    void OnShutdown(WorldRuntime&) override {}

    const WorldDebugState& State() const { return _state; }
    const std::string& Name() const { return _name; }

private:
    std::string _name;
    WorldDebugState _state{};
};

#include "WorldFactory.h"

class TestWorldFactory final : public IWorldFactory {
public:
    virtual ~TestWorldFactory() = default;
    virtual std::unique_ptr<IWorldImpl> CreateImpl(const WorldDesc& desc) override
    {
        static int id{ 1 };
        return std::make_unique<TestWorldImpl>("TestWorld" + std::to_string(id++));
    }
};

#include "ThreadPool.h"
#include "WorldRegistry.h"
#include "WorldScheduler.h"

int main()
{
    ThreadPool pool(4);
    TestWorldFactory factory;

    WorldRegistry reg(64, pool, factory);
    WorldScheduler sched(reg);

    WorldDesc descA;
    WorldDesc descB;
    WorldDesc descC;

    const WorldId wA = reg.CreateWorld(descA);
    const WorldId wB = reg.CreateWorld(descB);
    const WorldId wC = reg.CreateWorld(descC);

    sched.Register(wA, 60);
    sched.Register(wB, 30);
    sched.Register(wC, 10);

    using clock = std::chrono::steady_clock;
    auto prev = clock::now();
    auto end = prev + std::chrono::seconds(60);

    while (clock::now() < end)
    {
        auto now = clock::now();
        std::chrono::duration<double> dT{ now - prev };
        prev = now;

        sched.Update(dT.count());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    sched.Unregister(wA);
    sched.Unregister(wB);
    sched.Unregister(wC);

    if (auto* w = reg.GetWorld(wA)) w->Shutdown();
    if (auto* w = reg.GetWorld(wB)) w->Shutdown();
    if (auto* w = reg.GetWorld(wC)) w->Shutdown();

    reg.DestroyWorld(wA);
    reg.DestroyWorld(wB);
    reg.DestroyWorld(wC);

    return 0;
}