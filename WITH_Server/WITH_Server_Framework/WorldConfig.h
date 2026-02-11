#pragma once

// WorldConfig
//  - World 운영 중 조절 가능한 파라미터들
// 
// 1) Tick 관련 파라미터
// 2) 디버그/운영 플래그
// 3) 성능 튜닝 파라미터
// 4) 시스템 on/off 플래그

#include "types.h"

struct WorldConfig {
	uint32 tickRate{ 60 };
	bool pause{ false };
};