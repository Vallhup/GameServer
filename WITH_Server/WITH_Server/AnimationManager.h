#pragma once

#include <string_view>

#include "AnimationType.h"
#include "Constants.h"
#include "AnimationData.h"

class AnimationManager {
	struct AnimEntry
	{
		AnimationType anim{ AnimationType::None };
		bool loop{ false };
	};

	using AnimTable = std::array<AnimEntry, actionCnt* entityCnt* attackCnt>;

public:
	static AnimationManager& Get()
	{
		static AnimationManager instance;
		return instance;
	}

	void LoadAnimation(AnimationType type, std::string_view path);
	void LoadActionAnimationMap();
	const PrebakedAnimation* GetAnimation(AnimationType type) const;
	std::pair<AnimationType, bool> GetAnimationIdForAction(ActionType action, EntityType entity, AttackType attack) const;

private:
	AnimationManager() = default;
	PrebakedAnimation LoadPrebakedAnimation(std::string_view path);

	void Set(ActionType action, EntityType entity, AttackType attack, AnimationType anim, bool loop);
	const AnimEntry* Find(ActionType action, EntityType entity, AttackType attack) const;

	std::unordered_map<AnimationType, std::unique_ptr<PrebakedAnimation>> _animations;
	AnimTable _actionToAnimation;
};
