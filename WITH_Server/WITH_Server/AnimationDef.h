#pragma once

#include "AnimationId.h"
#include <cstdint>
#include <string>
#include <vector>
#include <DirectXMath.h>

using namespace DirectX;

enum class CapsuleRole : uint8_t
{
	None = 0,
	Hit,
	Hurt,
	Guard,
	Parry
};

struct AnimationCapsuleDef
{
	uint16_t boneIndex = 0;
	float radius;
	std::vector<CapsuleRole> roles;
};

struct Capsule
{
	XMFLOAT3 p0;
	XMFLOAT3 p1;
};

struct AnimationClipFrame
{
	std::vector<Capsule> capsules;
};

struct AnimationClipDef
{
	AnimationId id = AnimationId::None;
	std::string clipId;
	std::string skeleton;
	std::string source;

	uint16_t version = 3;
	float fps = 0.0f;
	uint16_t numFrames = 0;
	float durationSec = 0.0f;
	bool loop = false;
	std::string units = "cm";

	std::vector<AnimationCapsuleDef> capsuleDefs;
	std::vector<AnimationClipFrame> frames;
};
