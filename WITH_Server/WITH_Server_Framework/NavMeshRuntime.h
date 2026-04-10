#pragma once

#include <string>

// Detour forward declarations — 실제 정의는 NavMeshRuntime.cpp 내부에서만 포함
class dtNavMesh;
class dtNavMeshQuery;

// Recast/Detour NavMesh 런타임 래퍼.
// dtNavMesh(타일 데이터) + dtNavMeshQuery(쿼리 객체)의 생명주기를 함께 관리한다.
//
// [스레드 안전성]
// dtNavMeshQuery는 동시 호출에 안전하지 않다.
// NavMeshRuntime 인스턴스는 WorldRuntime당 1개이며,
// ResolveNavMeshBodyConstraintSystem은 해당 WorldRuntime 내에서 순차 실행되므로
// 별도 동기화 없이 안전하다.
class NavMeshRuntime final {
public:
    NavMeshRuntime() = default;
    ~NavMeshRuntime();

    NavMeshRuntime(const NavMeshRuntime&)            = delete;
    NavMeshRuntime& operator=(const NavMeshRuntime&) = delete;

    // 표준 Recast/Detour .bin 파일 포맷(NavMeshSetHeader + 타일 배열)을 로딩한다.
    // 성공 시 true, 파일 없음/포맷 불일치/Detour 초기화 실패 시 false.
    bool LoadFromFile(const std::string& binFilePath);

    void Unload();

    bool IsReady() const noexcept { return _navMesh != nullptr && _query != nullptr; }

    // 시스템에서 직접 쿼리를 수행할 때 사용
    dtNavMeshQuery* GetQuery() const noexcept { return _query; }

private:
    dtNavMesh*      _navMesh{ nullptr };
    dtNavMeshQuery* _query{ nullptr };
};
