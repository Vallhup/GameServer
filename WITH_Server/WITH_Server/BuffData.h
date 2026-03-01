#pragma once

#include "types.h"

enum class BuffType : uint8
{
	None,
	HpBoost,
	StaminaBoost,
	AttackBoost,
	AttackSpeedBoost,
	DefenceBoost,
	MoveSpeedBoost,

	Count
};

enum class BuffPolicy : uint8
{
	None,
	Duration,
	Stacks,
};

enum class BuffEffect : uint8
{
	None,
	AddStat,
	MulStat,
};

static constexpr size_t buffCnt{ static_cast<size_t>(BuffType::Count) };

struct BuffDef
{
	BuffType type;
	BuffPolicy policy;
	BuffEffect effect;
};

struct BuffInstance
{
	BuffType type{ BuffType::None };
	uint32 stackCount{ 0 };
	// 버프 시간 필요하면 추가
	// float remaining;
};