#pragma once

#include <DirectXMath.h>
#include "Component.h"

struct Velocity : public Component 
{
	DirectX::XMFLOAT3 dir{ 0, 0, 0 };
	bool isRun{ false };
};

struct ActionMoveDelta : public Component 
{
	bool hasMove{ false };
	DirectX::XMFLOAT3 deltaPos{ 0, 0,0 };
	bool hasYaw{ false };
	double yaw{ 0.0f };
};

struct LocomotionMoveDelta : public Component 
{
	bool hasMove{ false };
	DirectX::XMFLOAT3 deltaPos{ 0, 0,0 };
	bool hasYaw{ false };
	double yaw{ 0.0f };
};

struct LocomotionState : public Component 
{
	bool isMoving{ false };
	bool isRun{ false };
};