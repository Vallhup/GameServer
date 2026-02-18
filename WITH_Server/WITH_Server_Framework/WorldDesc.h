#pragma once

// WorldDesc
//  - World 구성에 필요한 불변값들
//  - 생성자 인자로 초기화 후 변경 불가
//
// 1) World Type
// 2) World Ruleset Id
//  - 규칙 셋, 스폰 테이블, AI 테이블 등
// 3) Capacity
//  - maxPlayers, maxEntities 등
// 4) Map Id
// 5) NetPolicy

#include "types.h"

struct WorldDesc {
	uint8 worldType;
	int worldRulesetId;
	uint64 Capacity;
	int mapId;
	uint8 netPolicy;
};