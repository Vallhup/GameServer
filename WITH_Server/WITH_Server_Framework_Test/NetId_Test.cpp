#include "pch.h"
#include <cassert>
#include <iostream>

#include "../../ProtocolLib/ProtocolLib/Include/NetId.h"
#include "NetIdRegistry.h"

#pragma comment(lib, "Asio_Network_Library.lib")

// ------------------------
// 테스트 유틸
// ------------------------
static void ExpectTrue(bool v) { assert(v); }
static void ExpectFalse(bool v) { assert(!v); }

static void ExpectNull(Entity e) { assert(e.IsNull()); }
static void ExpectEq(Entity a, Entity b) { assert(a == b); }

// ------------------------
// 테스트 1) Invalid 처리
// ------------------------
static void Test_InvalidHandling(NetIdRegistry& reg)
{
    ExpectNull(reg.FindEntity(NetId::Invalid()));

    // Invalid를 Free/Unbind해도 크래시/상태오염 없어야 함
    reg.Free(NetId::Invalid());
    ExpectFalse(reg.UnbindEntity(NetId::Invalid()));
}

// ------------------------
// 테스트 2) Allocate -> Bind -> Find -> Unbind -> Free 기본
// ------------------------
static void Test_BasicFlow(NetIdRegistry& reg)
{
    NetId id = reg.Allocate();
    ExpectTrue(id.IsValid());

    // IsAlive가 정상 return하는지 검증 (구현 누락이면 여기서 UB/랜덤)
    ExpectTrue(reg.IsAlive(id));

    Entity e = Entity{ 1234 }; // 프로젝트에 맞게 수정 (없으면 아래 대체)
    // ---- 대체(From이 없다면):
    // Entity e{1234};

    ExpectTrue(reg.BindEntity(id, e));
    ExpectEq(reg.FindEntity(id), e);

    ExpectTrue(reg.UnbindEntity(id));
    ExpectNull(reg.FindEntity(id));

    reg.Free(id);
    ExpectFalse(reg.IsAlive(id));
    ExpectNull(reg.FindEntity(id)); // Free 이후에도 Null이어야 안전
}

// ------------------------
// 테스트 3) 재사용 + generation mismatch 방어
// - old netId로 Find하면 Null이어야 함 (늦은 패킷 방어)
// ------------------------
static void Test_RecycleAndGenerationMismatch(NetIdRegistry& reg)
{
    NetId id1 = reg.Allocate();
    ExpectTrue(reg.IsAlive(id1));

    Entity e1 = Entity{ 111 };
    ExpectTrue(reg.BindEntity(id1, e1));
    ExpectEq(reg.FindEntity(id1), e1);

    reg.Free(id1);
    ExpectFalse(reg.IsAlive(id1));
    ExpectNull(reg.FindEntity(id1));

    NetId id2 = reg.Allocate();
    ExpectTrue(reg.IsAlive(id2));

    Entity e2 = Entity{ 222 };
    ExpectTrue(reg.BindEntity(id2, e2));
    ExpectEq(reg.FindEntity(id2), e2);

    // 핵심: old id1는 반드시 무효로 처리되어야 함
    ExpectNull(reg.FindEntity(id1));

    reg.Free(id2);
}

// ------------------------
// 테스트 4) Double Free 방어
// - 두 번째 Free가 free-list를 망가뜨리면 이후 Allocate에서 중복/오염이 발생
// ------------------------
static void Test_DoubleFree(NetIdRegistry& reg)
{
    NetId id = reg.Allocate();
    ExpectTrue(reg.IsAlive(id));

    reg.Free(id);
    ExpectFalse(reg.IsAlive(id));

    // 두 번째 Free는 무시되어야 함
    reg.Free(id);

    // 이후 Allocate가 정상적으로 동작해야 함
    NetId next = reg.Allocate();
    ExpectTrue(next.IsValid());
    ExpectTrue(reg.IsAlive(next));
    reg.Free(next);
}

// ------------------------
// 테스트 5) Bind 정책 확인(중복 Bind를 허용할지 여부)
// 당신 구현은 그냥 덮어쓰므로 "허용"입니다.
// - 그러면 최소한: 덮어쓴 후 Find가 마지막 entity를 반환해야 함
// ------------------------
static void Test_DoubleBindOverwritePolicy(NetIdRegistry& reg)
{
    NetId id = reg.Allocate();
    ExpectTrue(reg.IsAlive(id));

    Entity a = Entity{ 10 };
    Entity b = Entity{ 20 };

    ExpectTrue(reg.BindEntity(id, a));
    ExpectEq(reg.FindEntity(id), a);

    // 현재 구현은 overwrite
    ExpectTrue(reg.BindEntity(id, b));
    ExpectEq(reg.FindEntity(id), b);

    reg.Free(id);
}

// ------------------------
// main
// ------------------------
int main()
{
    NetIdRegistry reg(/*reserve=*/1024);

    std::cout << "Test1" << std::endl;
    Test_InvalidHandling(reg);

    std::cout << "Test2" << std::endl;
    Test_BasicFlow(reg);

    std::cout << "Test3" << std::endl;
    Test_RecycleAndGenerationMismatch(reg);

    std::cout << "Test4" << std::endl;
    Test_DoubleFree(reg);

    std::cout << "Test5" << std::endl;
    Test_DoubleBindOverwritePolicy(reg);

    std::cout << "[NetIdRegistryTests] All tests passed.\n";
    return 0;
}