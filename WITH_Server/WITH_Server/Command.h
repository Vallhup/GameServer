#pragma once

#include <DirectXMath.h>
#include <cstdint>

#include "Entity.h"
#include "Component.h"

#include "Constants.h"

enum class CommandSource : uint8_t
{
	None,
	Player,
	AI
};

struct MoveCommand
{
	bool hasMove{ false };
	bool moveRun{ false };
	DirectX::XMFLOAT3 moveDir{ 0, 0, 0 };

	inline void Clear()
	{
		hasMove = false;
		moveRun = false;
		moveDir = { 0, 0, 0 };
	}
};

struct LookCommand
{
	bool hasLook{ false };
	Entity target{ Entity::Null() };

	// TODO : 추후 필요하면 yaw 저장 추가

	inline void Clear()
	{
		hasLook = false;
		target = Entity::Null();
	}
};

struct ActionCommand
{
	bool hasAction{ false };
	ActionType actionType{ ActionType::None };
	AttackType attackType{ AttackType::None };

	uint32_t sequence{ 0 };

	inline void Clear()
	{
		hasAction = false;
		actionType = ActionType::None;
		attackType = AttackType::None;
		sequence = 0;
	}
};

struct GuardCommand
{
	bool hasGuard{ false };
	bool guardHeld{ false };

	inline void Clear()
	{
		hasGuard = false;
		guardHeld = false;
	}
};

struct EntityCommandFrame : Component
{
	CommandSource source{ CommandSource::None };

	MoveCommand move;
	LookCommand look;
	GuardCommand guard;
	ActionCommand action;

	inline void ClearFrameTransient()
	{
		if (!HasAnyCommand())
			source = CommandSource::None;

		guard = {};
		action = {};
	}

	inline void ClearAll()
	{
		source = CommandSource::None;
		move = {};
		look = {};
		guard = {};
		action = {};
	}

	inline bool HasAnyCommand() const
	{
		return move.hasMove || look.hasLook || guard.hasGuard || action.hasAction;
	}
};