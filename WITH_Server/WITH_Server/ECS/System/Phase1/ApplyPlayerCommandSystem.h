#pragma once

#include "System.h"

class ApplyPlayerCommandSystem final : public System {
	static const SystemMeta kMeta;

public:
	// 현재 WorldCommand는 사실상 PlayerCommand 밖에 없음
	// MonsterAICommand까지 WorldCommand에 포함시키고 매핑 테이블 만들어서 Phase 1에서 처리할 수 있도록
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMeta; }
};
