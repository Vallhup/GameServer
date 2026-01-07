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
	Knight_Death,
	Knight_Drinking,
	Knight_Guard,
	Knight_Hit,
	Knight_Parry,
	Knight_Roll,
	Knight_Stun,
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

struct Capsule {
	int bone{ 0 };
	XMFLOAT3 p0;
	XMFLOAT3 p1;
	float radius{ 0.0f };

	XMVECTOR P0() const { return XMLoadFloat3(&p0); }
	XMVECTOR P1() const { return XMLoadFloat3(&p1); }
};

struct PrebakedAnimation {
	float fps;
	int numFrames;
	std::vector<std::vector<Capsule>> frames;
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

