#pragma once

#include "System.h"
#include "SystemMetaStorage.h"

// ---------------------------------------------------------------------------
// RecordCombatStatisticsSystem
//
// 처치/사망 이벤트 컴포넌트(PendingMonsterKillEventComp,
// PendingPlayerDeathCountEventComp)를 소비하여 DB 커맨드를 발행한다.
//
// 실행 순서:
//   CommitCombatResultSystem  → 이벤트 세팅
//   ResolveDeathAndDespawnSystem → 사망 상태 기계 전진 + pending 세팅
//   RecordCombatStatisticsSystem  → 이벤트 소비 + DB 커맨드 발행
// ---------------------------------------------------------------------------
class RecordCombatStatisticsSystem final : public System
{
	static const StaticSystemMetaStorage<3> kMetaStorage;

public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }
};
