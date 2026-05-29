#pragma once

#include "DynamicTaskTypes.h"

// ---------------------------------------------------------------------------
// 전투 통계 DB 완료 핸들러 타입 ID
//
// RegisterServerPacketHandlers() 내에서 채워진다. 그 이전에는 모두
// InvalidDynamicTaskTypeId(0)이다.
//
// RecordCombatStatisticsSystem 등 ECS 시스템에서 DB 커맨드 제출 시
// DBRequestMeta.completionTaskTypeId 에 사용한다.
// ---------------------------------------------------------------------------
extern DynamicTaskTypeId g_incrementMonsterKillCountResultTaskTypeId;
extern DynamicTaskTypeId g_incrementDeathByMonsterCountResultTaskTypeId;
extern DynamicTaskTypeId g_unlockTitleResultTaskTypeId;
