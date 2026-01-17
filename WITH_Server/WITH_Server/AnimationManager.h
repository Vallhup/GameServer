#pragma once

#include <string_view>

#include "json.hpp"

using json = nlohmann::json;
using namespace DirectX;

enum class AnimationId {
	None,
	Knight_Idle,
	Knight_Walk,
	Knight_Run,
	Knight_Attack,
	Knight_Dead,
	Knight_Drinking,
	Knight_Guard,
	Knight_Hit,
	Knight_Parry,
	Knight_Dodge,
	Knight_Stun,
};

enum class HitboxType : uint8 {
	None    = 0,
	Hurt	= 1 << 0,
	Hit     = 1 << 1,
	Guard   = 1 << 2,
	Parry	= 1 << 3
};

namespace std {
	template<>
	struct hash<AnimationId> {
		size_t operator()(const AnimationId& id) const noexcept
		{
			return std::hash<int>()(static_cast<int>(id));
		}
	};
}

struct DynamicCapsuleData {
	XMFLOAT3 p0{ 0, 0, 0 };
	XMFLOAT3 p1{ 0, 0, 0 };
};

struct StaticCapsuleData {
	uint8 bone{ 0 };
	float radius{ 0.0f };
	uint8 typeMask{ static_cast<uint8>(HitboxType::None) };
};

struct PrebakedAnimation {
	float fps;
	uint8 numFrames;

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

	void LoadAnimation(AnimationId id, std::string_view path);
	const PrebakedAnimation* GetAnimation(AnimationId id) const;

private:
	AnimationManager() = default;
	PrebakedAnimation LoadPrebakedAnimation(std::string_view path);

	std::unordered_map<AnimationId, std::unique_ptr<PrebakedAnimation>> _animations;
};
