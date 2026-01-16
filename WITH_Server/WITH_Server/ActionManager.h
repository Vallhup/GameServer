#pragma once

enum class ActionType {
	None,
	Attack,
	Dodge,
	Parry,
	Hit,
	Guard,
	Stun,
	Dead
};

namespace std {
	template<>
	struct hash<ActionType> {
		size_t operator()(const ActionType& id) const noexcept
		{
			return std::hash<int>()(static_cast<int>(id));
		}
	};
}

struct ActionMoveSegment {
	float t0;
	float t1;
	float distance;
	bool lockDir;
};

struct ActionProfile {
	std::vector<ActionMoveSegment> segments;
};

class ActionManager {
public:
	static ActionManager& Get()
	{
		static ActionManager instance;
		return instance;
	}

	void LoadAction(ActionType id, std::string_view path);
	const ActionProfile* GetActionMoveProfile (ActionType actionType) const;

private:
	ActionManager();
	ActionProfile LoadActionProfile(std::string_view path);

	std::unordered_map<ActionType, std::unique_ptr<ActionProfile>> _actionProfiles;
};

