#pragma once

#include "AnimationManager.h"
#include "ActionManager.h"
#include "Entity.h"

using namespace DirectX;

struct Component {
public:
	virtual ~Component() = default;
};

struct Transform : public Component {
	XMFLOAT3 position{ 0, 0, 0 };
	XMFLOAT4 rotation{ 0, 0, 0, 1 };
	XMFLOAT3 scale{ 1, 1, 1 };
};

struct Velocity : public Component {
	XMFLOAT3 dir{ 0, 0, 0 };
	XMFLOAT3 lastNonZeroDir{ 0, 0, 0 };
	bool isRun{ false };
};

struct ActionMoveDelta : public Component {
	bool hasMove{ false };
	XMFLOAT3 deltaPos{ 0, 0,0 };
	bool hasYaw{ false };
	float yaw{ 0.0f };
};

struct LocomotionMoveDelta : public Component {
	bool hasMove{ false };
	XMFLOAT3 deltaPos{ 0, 0,0 };
	bool hasYaw{ false };
	float yaw{ 0.0f };
};

struct LocomotionAnimPhase : public Component {
	float phase{ 0.0f };
	bool wasMoving{ false };
};

struct LocomotionState : public Component {
	bool isMoving{ false };
};

struct ActionIntent : public Component {
	bool attack{ false };
	bool dodge{ false };
	bool parry{ false };
	bool guard{ false };
};

struct ActionState : public Component {
	ActionType type{ ActionType::None };
	float elapsed{ 0.0f };
	float duration{ 0.0f };
};

struct AttackData : public Component {
	int damage{ 10 };
};

struct Health : public Component {
	int current{ 100 };
	int max{ 100 };
};

struct AnimationState : public Component {
	AnimationId desiredId{ AnimationId::Knight_Idle };
	float speed{ 1.0f };
	bool looping{ true };
};

struct Animator : public Component {
	const PrebakedAnimation* clip{ nullptr };
	uint16 currentFrame{ 0 };
};

struct Collider : public Component {
	const std::vector<StaticCapsuleData>* staticDatas{ nullptr };

	std::vector<DynamicCapsuleData> localDatas;
	std::vector<DynamicCapsuleData> worldDatas;

	std::vector<uint8> enabledMasks;
	std::vector<uint32> attackIds;

	uint32 staticCount{ 0 };
};

struct AttackState : public Component {
	uint32 attackId{ 0 };
	ActionType prevAction{ ActionType::None };
};

struct ParryBuf : public Component {
	int remaining{ 0 };
	// TEMP : Parry 성공 시 추가 데미지
	float additionalDamage{ 1.0f };
};

struct DisconnectedTag :public Component { };

struct ActionMoveTag : public Component {
	const ActionProfile* profile{ nullptr };

	uint8 segmentIndex{ 0 };
	float movedInSegment{ 0.0f };

	XMFLOAT3 dir{ 0, 0, 0 };
	bool dirLocked{ false };
};