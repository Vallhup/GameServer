#pragma once

#include "ActionData.h"
#include "Constants.h"

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

