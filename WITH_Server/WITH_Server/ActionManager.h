#pragma once

#include "types.h"
#include "AnimationManager.h"
#include "Constants.h"

enum class MoveMode : uint8
{
	None,

	FixedDistance,
	DashToTarget
};

struct MoveParams
{
	float maxSpeed{ 0.0f };
	float maxTravel{ 0.0f };
	float stopRange{ 0.0f };

	float distance{ 0.0f };

	bool lockDir{ false };
};

struct ActionMoveSegment {
	double t0;
	double t1;
	MoveMode mode;
	MoveParams params;
};

struct ActionProfile {
	std::vector<ActionMoveSegment> segments;
};

struct ActionPolicy {
	int32 priority{ 0 };
	double duration{ 0 };
	uint32 interruptMask{ 0 };
	bool isMoveAction{ false };
	bool isHoldAction{ false };

	constexpr bool IsValid() const { return priority == 0 && duration == 0 && interruptMask == 0; }
};

class ActionManager {
	using PolicyTable = std::array<ActionPolicy, actionCnt* entityCnt* attackCnt>;
	using ProfileTable = std::array<ActionProfile, actionCnt* entityCnt* attackCnt>;

public:
	static ActionManager& Get()
	{
		static ActionManager instance;
		return instance;
	}

	void LoadAction(ActionType id, std::string_view path);

	const ActionPolicy& GetPolicy(ActionType action, EntityType entity, AttackType attack) const;
	const ActionProfile* GetActionMoveProfile (ActionType action, EntityType entity, AttackType attack) const;

private:
	ActionManager();

	void LoadPolicy();
	void LoadProfile();
	ActionProfile LoadActionProfile(std::string_view path);

	void SetPolicy(ActionType action, EntityType entity, AttackType attack, const ActionPolicy& policy);
	void SetProfile(ActionType action, EntityType entity, AttackType attack, const ActionProfile& profile);

	const ActionPolicy* FindPolicy(ActionType action, EntityType entity, AttackType attack) const;
	const ActionProfile* FindProfile(ActionType action, EntityType entity, AttackType attack) const;

	PolicyTable _policyTable;
	ProfileTable _profileTable;
};

