#pragma once

#include <DirectXMath.h>
#include "ActionData.h"
#include "Component.h"
#include "Constants.h"
#include "Entity.h"

struct ActionState : public Component {
	ActionType action{ ActionType::None };
	AttackType attack{ AttackType::None };
	double elapsed{ 0.0f };
	double duration{ 0.0f };
	double progress{ 0.0f };
};

struct AttackState : public Component {
	uint32 attackId{ 0 };
	AttackType type{ AttackType::None };
	ActionType prevAction{ ActionType::None };

	std::array<Entity, 2> hitVictims;
	uint8 hitCount{ 0 };

	bool HasHit(Entity e) const
	{
		for (uint8 i = 0; i < hitCount; ++i)
			if (hitVictims[i] == e) return true;

		return false;
	}

	void MarkHit(Entity e)
	{
		if (hitCount < hitVictims.size())
			hitVictims[hitCount++] = e;
	}
};

struct ActionMoveTag : public TagComponent {
	const ActionProfile* profile{ nullptr };

	int8 segmentIndex{ 0 };
	double movedInSegment{ 0.0 };
	double vMovedInSegment{ 0.0 };

	DirectX::XMFLOAT3 dir{ 0, 0, 0 };
	bool dirLocked{ false };

	float yaw{ 0.0f };
	bool yawLocked{ false };

	int8 lastSegmentIndex{ -1 };
	bool dashHasTarget{ false };
	DirectX::XMFLOAT3 dashTargetPos{ 0, 0, 0 };
	double dashTraveled{ 0.0 };
	DirectX::XMFLOAT3 dashStartPos{ 0, 0, 0 };
	double dashTotalDist{ 0.0 };
};