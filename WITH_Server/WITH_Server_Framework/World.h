#pragma once

#include "WorldId.h"
#include "WorldDesc.h"
#include "WorldConfig.h"
#include "WorldRuntime.h"

class IWorld {
public:
	virtual ~IWorld() = default;

	virtual void Init() = 0;
	virtual void Update(const float dT) = 0;
	virtual void Shutdown() = 0;
};

class IWorldImpl {
public:
	virtual ~IWorldImpl() = default;

	virtual void Build(WorldRuntime& rt) = 0;

	virtual void ApplyInbox(WorldRuntime& rt, float dT) = 0;
	virtual void Execute(WorldRuntime& rt, float dT) = 0;
	virtual void BuildOutbox(WorldRuntime& rt, float dT) = 0;
	virtual void FlushOutbox(WorldRuntime& rt, float dT) = 0;

	virtual void OnShutdown(WorldRuntime& rt) = 0;
};

class World final : public IWorld {
public:
	World(WorldId id, const WorldDesc& desc, ThreadPool& pool,
		std::unique_ptr<IWorldImpl> impl);

	virtual void Init() override;
	virtual void Update(const float dT) override;
	virtual void Shutdown() override;

private:
	WorldId _id;
	WorldDesc _desc;

	WorldRuntime _runtime;
	std::unique_ptr<IWorldImpl> _impl;
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