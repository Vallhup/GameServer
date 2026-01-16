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
	AnimationId id{ AnimationId::Knight_Idle };
	//float time{ 0.0f };
	float speed{ 1.0f };
	bool looping{ true };
};

struct AnimationRef : public Component {
	const PrebakedAnimation* anim{ nullptr };
};

struct Animator : public Component {
	int currentFrame{ 0 };
};

struct Collider : public Component {
	std::vector<Capsule> localCapsules;
	std::vector<Capsule> worldCapsules;
};

struct DisconnectedTag :public Component { };

struct HitTag : public Component {
	int damage{ 0 };
	Entity attacker;
	bool invalid{ false };
};

struct ActionRequestTag : public Component {
	ActionType type;
};

struct ParryBuff : public Component {
	int remaining{ 1 };
	// TEMP : Parry 성공 시 추가 데미지
	float additionalDamage{ 1.0f };
};

struct ActionMoveTag : public Component {
	const ActionProfile* profile{ nullptr };
	
	float elapsed{ 0.0f };
	uint8 segmentIndex{ 0 };
	float movedInSegment{ 0.0f };

	XMFLOAT3 dir{ 0, 0, 0 };
	bool dirLocked{ false };
};