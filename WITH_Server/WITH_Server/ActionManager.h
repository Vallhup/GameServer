#pragma once

enum class AttackType : uint8 {
	None,

	// Player
	Light,
	Heavy,

	// Boss
	JumpSlam,
	FarWaveSlash,
	DashSlash,
	Thrust,
	CloseSlash,
	Meteor,
};

enum class ActionType : uint8 {
	None,
	Attack,
	Dodge,
	Parry,
	Stun,
	Hit,
	Guard,
	Dead,
	Count
};

namespace std {
	template<>
	struct hash<ActionType> {
		size_t operator()(const ActionType& id) const noexcept
		{
			return std::hash<uint8>()(static_cast<uint8>(id));
		}
	};
}

constexpr size_t ToIndex(ActionType type) { return static_cast<size_t>(type); }
constexpr size_t ActionCount = ToIndex(ActionType::Count);

constexpr uint32 Bit(ActionType type) { return (uint32)1u << ToIndex(type); }

struct ActionMoveSegment {
	float t0;
	float t1;
	float distance;
	bool lockDir;
};

struct ActionProfile {
	std::vector<ActionMoveSegment> segments;
};

struct ActionPolicy {
	int32 priority{ 0 };
	float duration{ 0 };
	uint32 interruptMask{ 0 };
	bool isMoveAction{ false };
	bool isHoldAction{ false };
};

class ActionManager {
public:
	static ActionManager& Get()
	{
		static ActionManager instance;
		return instance;
	}

	void LoadAction(ActionType id, std::string_view path);

	const ActionPolicy& GetPolicy(ActionType type) const;
	const ActionProfile* GetActionMoveProfile (ActionType actionType) const;

private:
	ActionManager();

	void LoadPolicy();
	void LoadProfile();
	ActionProfile LoadActionProfile(std::string_view path);

	std::array<ActionPolicy, ActionCount> _policies;
	std::unordered_map<ActionType, std::unique_ptr<ActionProfile>> _actionProfiles;
};

