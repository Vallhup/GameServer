#include "pch.h" // PCH 안 쓰면 삭제
#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>
#include <algorithm>

struct A : Component { int v = 1; };
struct B : Component { int v = 2; };
struct Dead : TagComponent {};

static volatile std::uint64_t g_sink = 0;

using bench_clock = std::chrono::steady_clock;

template<typename F>
double BenchMs(const char* name, int iters, F&& fn)
{
    // 워밍업
    for (int i = 0; i < 3; ++i) fn();

    auto t0 = bench_clock::now();
    for (int i = 0; i < iters; ++i) fn();
    auto t1 = bench_clock::now();

    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::cout << name << " : " << (ms / iters) << " ms/iter\n";
    return ms / iters;
}

int main()
{
    ECS ecs;

    constexpr int N = 2'000'000;   // 엔티티 수
    constexpr int NB = 200'000;    // B 보유 수 (10%)
    constexpr int NDead = 50'000;  // Dead 보유 수

    std::vector<Entity> es;
    es.reserve(N);
    for (int i = 0; i < N; ++i)
        es.push_back(ecs.CreateEntity());

    // A는 전부
    auto& Astore = ecs.GetStorage<A>();
    for (int i = 0; i < N; ++i) {
        Astore.AddComponent(es[i])->v = i;
    }

    // B는 일부만
    auto& Bstore = ecs.GetStorage<B>();
    for (int i = 0; i < NB; ++i) {
        Bstore.AddComponent(es[i])->v = i * 3;
    }

    // Dead는 일부
    auto& Dstore = ecs.GetStorage<Dead>();
    for (int i = 0; i < NDead; ++i) {
        Dstore.AddComponent(es[i * 2]); // 구현에 맞게 (포인터/레퍼런스) 수정
    }

    // --------- 비교군: 수동 루프 (B 기준) ----------
    auto manual_join = [&]() {
        std::uint64_t local = 0;
        // B가 적으니까 B dense를 직접 돈다고 가정(당신 ComponentStorage가 DenseEntityAt 지원하면 더 좋음)
        // 여기선 View의 smallest-first와 동일한 철학: "작은 풀 기준"
        const IStorage& base = static_cast<const IStorage&>(Bstore);
        const size_t n = base.Size();

        for (size_t i = 0; i < n; ++i) {
            Entity e = base.EntityAt(i);
            if (!Astore.HasComponent(e)) continue;

            auto* a = Astore.GetComponent(e);
            auto* b = Bstore.GetComponent(e);
            if (!a || !b) continue; // 방어

            local += (std::uint64_t)a->v + (std::uint64_t)b->v;
        }
        g_sink += local;
        };

    // --------- 대상: View<A,B> ----------
    auto view_join = [&]() {
        std::uint64_t local = 0;
        for (auto [e, a, b] : ecs.View<A, B>()) {
            (void)e;
            local += (std::uint64_t)a.v + (std::uint64_t)b.v;
        }
        g_sink += local;
        };

    // --------- 대상: View<A,B> Exclude<Dead> ----------
    auto view_join_ex = [&]() {
        std::uint64_t local = 0;
        for (auto [e, a, b] : ecs.View<A, B>(Exclude<Dead>{})) {
            (void)e;
            local += (std::uint64_t)a.v + (std::uint64_t)b.v;
        }
        g_sink += local;
        };

    // 반복 횟수: N이 크면 3~10회면 충분
    BenchMs("manual_join(B base)", 5, manual_join);
    BenchMs("view_join", 5, view_join);
    BenchMs("view_join_ex", 5, view_join_ex);

    std::cout << "sink=" << (std::uint64_t)g_sink << "\n";
}