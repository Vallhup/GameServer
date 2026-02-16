#pragma once

#include "AnimationManager.h"
#include "ActionManager.h"
#include "Entity.h"
#include "Component.h"

using namespace DirectX;

struct Transform : public Component {
	XMFLOAT3 position{ 10.0f, 0, 10.0f };
	XMFLOAT4 rotation{ 0, 0, 0, 1 };
	XMFLOAT3 scale{ 1, 1, 1 };
};

struct Velocity : public Component {
	XMFLOAT3 dir{ 0, 0, 0 };
	bool isRun{ false };
};

struct ActionMoveDelta : public Component {
	bool hasMove{ false };
	XMFLOAT3 deltaPos{ 0, 0,0 };
	bool hasYaw{ false };
	double yaw{ 0.0f };
};

struct LocomotionMoveDelta : public Component {
	bool hasMove{ false };
	XMFLOAT3 deltaPos{ 0, 0,0 };
	bool hasYaw{ false };
	double yaw{ 0.0f };
};

struct LocomotionAnimPhase : public Component {
	double phase{ 0.0f };
	bool wasMoving{ false };
};

struct LocomotionState : public Component {
	bool isMoving{ false };
	bool isRun{ false };
};

struct ActionIntent : public Component {
	bool attack{ false };
	bool dodge{ false };
	bool parry{ false };
	bool guard{ false };
};

struct AIState : public Component {
	Entity target;
	Entity lastAttacker;
	int patternsOnTarget;
};

struct AIThinkState : public Component {
	double thinkAcc{ 0.0f };
	double thinkInterval{ 5.0f };
};

struct ActionState : public Component {
	ActionType type{ ActionType::None };
	double elapsed{ 0.0f };
	double duration{ 0.0f };
};

struct AttackData : public Component {
	int damage{ 10 };
};

struct Health : public Component {
	int current{ 100 };
	int max{ 100 };
};

struct AnimationState : public Component {
	AnimationType desiredId{ AnimationType::Knight_Idle };
	double speed{ 1.0f };
	bool looping{ true };
};

struct Animator : public Component {
	const PrebakedAnimation* clip{ nullptr };
	uint16 currentFrame{ 0 };
};

struct AABB {
	XMFLOAT3 min;
	XMFLOAT3 max;
};

struct CombatCollider : public Component {
	const std::vector<StaticCapsuleData>* staticDatas{ nullptr };
	std::vector<DynamicCapsuleData> localDatas;
	std::vector<DynamicCapsuleData> worldDatas;

	std::vector<uint8> enabledMasks;
	std::vector<uint32> attackIds;
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

struct ViewList : public Component {
	std::vector<uint32> viewList;
};

struct ParryBuf : public Component {
	int remaining{ 0 };
	// TEMP : Parry 성공 시 추가 데미지
	double additionalDamage{ 1.0f };
};

struct DisconnectedTag :public TagComponent { };
struct PlayerTag :public TagComponent { };

struct ActionMoveTag : public TagComponent {
	const ActionProfile* profile{ nullptr };

	uint8 segmentIndex{ 0 };
	double movedInSegment{ 0.0f };

	XMFLOAT3 dir{ 0, 0, 0 };
	bool dirLocked{ false };
};