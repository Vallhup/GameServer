#pragma once

#include "AnimationId.h"
#include <string>
#include <DirectXMath.h>

using namespace DirectX;

enum class CapsuleRole : uint8_t
{
	None,
	Hit,
	Hurt
};

struct AnimationCapsuleDef
{
	uint8_t boneIndex;
	float radius;
	std::vector<CapsuleRole> roles;
};

struct Capsule
{
	XMFLOAT3 p0;
	XMFLOAT3 p1;
};

struct AnimationCapsuleFrame
{
	uint16_t frameIndex;
	std::vector<Capsule> capsules;
};

struct AnimationCollisionDef
{
	AnimationId id;
	std::string name;

	uint16_t version;
	uint8_t fps;
	uint16_t numFrames;

	std::vector<AnimationCapsuleDef> capsuleDefs;
	std::vector<AnimationCapsuleFrame> capsuleFrames;
};	