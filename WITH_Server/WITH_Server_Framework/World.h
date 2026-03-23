#pragma once

#include "WorldId.h"
#include "WorldDesc.h"
#include "WorldConfig.h"
#include "WorldRuntime.h"
#include "PlayerSnapshot.h"

#include <unordered_set>

enum class PlayerPresenceState : uint8_t
{
	None,
	PendingSpawn,
	Active,
	PendingDespawn
};

class IWorld {
public:
	virtual ~IWorld() = default;

	virtual void Init() = 0;
	virtual void Update(const double dT) = 0;
	virtual void Shutdown() = 0;
};

class WorldBuilder;

class IWorldImpl {
public:
	virtual ~IWorldImpl() = default;

	virtual void Configure(WorldBuilder& builder) = 0;

	virtual Entity SpawnPlayer(WorldRuntime& rt, uint32_t connId) = 0;
	virtual bool DespawnPlayer(WorldRuntime& rt, uint32_t connId) { return false; };

	virtual bool TryMakeSnapshot(WorldRuntime& rt, uint32_t connId, PlayerSnapshot& out) { return false; };
	virtual bool ApplySnapshot(WorldRuntime& rt, uint32_t connId, const PlayerSnapshot& snapshot) { return false; };

	virtual void OnShutdown(WorldRuntime& rt) {};

};

class World final : public IWorld {
public:
	World(WorldId id, const WorldDesc& desc, ThreadPool& pool,
		std::unique_ptr<IWorldImpl> impl);

	virtual void Init() override;
	virtual void Update(const double dT) override;
	virtual void Shutdown() override;

	Entity RequestSpawnPlayer(uint32_t connId);
	bool RequestDespawnPlayer(uint32_t connId);

	bool TryMakeSnapshot(WorldRuntime& rt, uint32_t connId, PlayerSnapshot& out);
	bool ApplySnapshot(WorldRuntime& rt, uint32_t connId, const PlayerSnapshot& snapshot);

	bool HasPresence(uint32_t connId) const;
	bool HasPlayer(uint32_t connId) const;
	bool IsPlayerPendingSpawn(uint32_t connId) const;
	bool IsPlayerPendingDespawn(uint32_t connId) const;

	void FinalizePresence();

	WorldRuntime& Runtime() { return _runtime; }
	const WorldRuntime& Runtime() const { return _runtime; }

private:
	struct PlayerPresenceEntry
	{
		PlayerPresenceState state{ PlayerPresenceState::None };
		Entity entity{};
	};

	WorldId _id;
	WorldDesc _desc;

	WorldRuntime _runtime;
	std::unique_ptr<IWorldImpl> _impl;

	std::unordered_map<uint32_t, PlayerPresenceEntry> _players;
};

// World 구조 및 구현 상 특징 예상
// - Entity ID는 World 단위로 고유 식별자 부여
// - World별 설정값: TickRate, SystemGraphPreset, ReplicationPreset 등
//
// - World Lifecycle 관리: 생성, 초기화, 업데이트, 종료 등
//   - WorldDesc(맵, maxPlayers, tickRate등 정적 데이터)
//   - WorldRegistry(생성/소멸)
//   - WorldScheduler(스케줄링)
//   - 보스 처치/실패 시 월드 종료 및 결과 처리
// 
// - World 이동 = Snapshot + Recreate
//   - 월드 A에서 결과 Snapshot 생성(캐릭터 상태, 스탯, 아이템 등)
//   - 월드 B에서 Snapshot을 기반으로 Entity 재생성
//   - 월드 간 동기화: 클리어/사망 등만 전달 / 상세 이벤트, 상태 동기화 X
// 
// - NetId. SessionId, EntityId 관리
//   - WorldId를 별도 필드로 관리
// 
// 1. Login
//  - ECS 필요 없음
//  - TickRate: 필요 없음
// 
// 2. Squre - 동접 5000명 목표
//  - SystemGraphPreset: 이동, 근거리 상호작용, 파티매칭용 로직 등
//  - ReplicationPreset: 섹터, TickRate 낮게
// 
// 3. Start - 동접 1, 몬스터 3 ~ 5
// 
// 4. Middle - 동접 3, 몬스터 3 ~ 5
// 
// 5. Final - 동접 3, 몬스터 1
// 
// 6. PVP - 동접 3



// 2026. 03. 17 코드 리뷰 결과
//
// 장점
//  1. 명확한 아키텍처 축과 책임 분리
// 
// 단점
//  1. 운영 안정성
//   - 예외 처리, 틱 드리프트 방지, 실패 복구 등
//  
//  2. Event 모듈의 완성도 부족
//   - 이름만 붙인 임시 버퍼에 불과한 수준
// 
// 개선 우선순위
//  1. EventQueue 계약 재설계
//   - ConsumeView의 계약 재설계
//   - consume / double-buffer frame event / persistent log
// 
//  2. WorldScheduler fixed timestep 수정 (완료)
//   - 틱 드리프트 방지를 위한 capped substep loop 필요
// 
//  3. WorldLeapRequest 표현 수정
//   - std::array<uint32_t, 3>이 아닌 
//     실제 player count가 드러나는 구조로 수정
// 
//  4. WorldRegistry 생성 rollback 추가
//   - factory null check
//   - init failed
//   - id 반환 취소 등
// 
//  5. WorldService 설정 데이터화
//   - 하드코딩 제거
//   - WorldDesc가 의미를 가지도록 
// 
//  6. WorldIdAllocator Type 일치