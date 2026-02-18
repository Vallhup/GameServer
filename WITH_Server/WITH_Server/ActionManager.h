#pragma once

#include "types.h"
#include "AnimationManager.h"
#include "Constants.h"

struct ActionMoveSegment {
	double t0;
	double t1;
	double distance;
	bool lockDir;
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
	using ActionTable = std::array<ActionPolicy, actionCnt* entityCnt* attackCnt>;

public:
	static ActionManager& Get()
	{
		static ActionManager instance;
		return instance;
	}

	void LoadAction(ActionType id, std::string_view path);

	const ActionPolicy& GetPolicy(ActionType action) const;
	const ActionPolicy& GetPolicy(ActionType action, EntityType entity, AttackType attack) const;
	const ActionProfile* GetActionMoveProfile (ActionType actionType) const;

private:
	ActionManager();

	void LoadPolicy();
	void LoadProfile();
	ActionProfile LoadActionProfile(std::string_view path);

	void SetPolicy(ActionType action, EntityType entity, AttackType attack, const ActionPolicy& policy);
	const ActionPolicy* Find(ActionType action, EntityType entity, AttackType attack) const;

	std::array<ActionPolicy, actionCnt> _policies;
	ActionTable _policyTable;
	std::unordered_map<ActionType, std::unique_ptr<ActionProfile>> _actionProfiles;
};

