#pragma once

#include <DirectXMath.h>
#include "types.h"

enum class HitboxType : uint8 {
	None = 0,
	Hurt = 1 << 0,
	Hit = 1 << 1,
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
	DirectX::XMFLOAT3 p0{ 0, 0, 0 };
	DirectX::XMFLOAT3 p1{ 0, 0, 0 };

	DirectX::XMVECTOR P0() const { return XMLoadFloat3(&p0); }
	DirectX::XMVECTOR P1() const { return XMLoadFloat3(&p1); }
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
