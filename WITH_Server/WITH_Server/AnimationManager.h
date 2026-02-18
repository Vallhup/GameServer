#pragma once

#include <DirectXMath.h>
#include <string_view>

#include "json.hpp"
#include "AnimationType.h"
#include "Constants.h"

using json = nlohmann::json;
using namespace DirectX;

enum class HitboxType : uint8 {
	None    = 0,
	Hurt	= 1 << 0,
	Hit     = 1 << 1,
};

inline uint8 operator|(HitboxType a, HitboxType b)
{
	return static_cast<uint8>(a) | static_cast<uint8>(b);
}

inline bool HasType(uint8 mask, HitboxType t)
{
	return (mask & static_cast<uint8>(t)) != 0;
}

struct DynamicCapsuleData {
	XMFLOAT3 p0{ 0, 0, 0 };
	XMFLOAT3 p1{ 0, 0, 0 };

	XMVECTOR P0() const { return XMLoadFloat3(&p0); }
	XMVECTOR P1() const { return XMLoadFloat3(&p1); }
};

struct StaticCapsuleData {
	uint8 bone{ 0 };
	double radius{ 0.0f };
	uint8 typeMask{ static_cast<uint8>(HitboxType::None) };
};

struct PrebakedAnimation {
	double fps{ 0.0 };
	uint16 numFrames{ 0 };

	std::vector<StaticCapsuleData> staticDatas;
	std::vector<std::vector<DynamicCapsuleData>> dynamicDatas;
};

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
	std::pair<AnimationType, bool> GetAnimationIdForAction(ActionType action) const;
	std::pair<AnimationType, bool> GetAnimationIdForAction(ActionType action, EntityType entity, AttackType attack) const;

private:
	AnimationManager() = default;
	PrebakedAnimation LoadPrebakedAnimation(std::string_view path);

	void Set(ActionType action, EntityType entity, AttackType attack, AnimationType anim, bool loop);
	const AnimEntry* Find(ActionType action, EntityType entity, AttackType attack) const;

	std::unordered_map<AnimationType, std::unique_ptr<PrebakedAnimation>> _animations;
	std::unordered_map<ActionType, std::pair<AnimationType, bool>> _actionToAnimationMap;
	AnimTable _actionToAnimation;
};
