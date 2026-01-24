#pragma once

#include <string_view>

#include "json.hpp"
#include "AnimationType.h"
#include "ActionManager.h"

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
	float radius{ 0.0f };
	uint8 typeMask{ static_cast<uint8>(HitboxType::None) };
};

struct PrebakedAnimation {
	float fps;
	uint16 numFrames;

	std::vector<StaticCapsuleData> staticDatas;
	std::vector<std::vector<DynamicCapsuleData>> dynamicDatas;
};

class AnimationManager {
public:
	static AnimationManager& Get()
	{
		static AnimationManager instance;
		return instance;
	}

	void LoadAnimation(AnimationType type, std::string_view path);
	void LoadActionAnimationMap();
	const PrebakedAnimation* GetAnimation(AnimationType type) const;
	std::pair<AnimationType, bool> GetAnimationIdForAction(
		ActionType action) const;

private:
	AnimationManager() = default;
	PrebakedAnimation LoadPrebakedAnimation(std::string_view path);

	std::unordered_map<AnimationType, std::unique_ptr<PrebakedAnimation>> _animations;
	std::unordered_map<ActionType, std::pair<AnimationType, bool>> _actionToAnimationMap;
};
