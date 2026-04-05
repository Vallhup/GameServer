#pragma once

#include <cstdint>

#include "IDs.h"
#include "WorldCommand.h"

// AI 명령 타입 키 (PlayerCommandTypeKey와 별도 공간 사용)
// AI는 Move + Action만 존재. Dodge/Guard/Parry 없음.
// Action 종류는 payload의 ActionId로 결정 (AI 타입별 확장 가능).
enum class AICommandTypeKey : WorldCommandTypeKey
{
	None   = 0,
	Move   = 100,
	Action = 101
};

struct AIMoveCommandPayload
{
	float inputX{ 0.0f };
	float inputZ{ 0.0f };
	float yaw{ 0.0f };
	uint8_t isRun{ 0 };
};

struct AIActionCommandPayload
{
	ActionId actionId{ ActionId::None };
	float dirX{ 0.0f };
	float dirZ{ 0.0f };
};

bool IsAIMoveCommandType(WorldCommandTypeKey typeKey) noexcept;
bool IsAIActionCommandType(WorldCommandTypeKey typeKey) noexcept;
