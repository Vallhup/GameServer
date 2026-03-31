#pragma once

#include <DirectXMath.h>
#include "Component.h"

struct Transform : public Component {
	DirectX::XMFLOAT3 position{ -7.0f, 10.0f, -242.0f };		//-70.0f, -350.0f, -400.0f
	DirectX::XMFLOAT4 rotation{ 0, 0, 0, 1 };
	DirectX::XMFLOAT3 scale{ 1, 1, 1 };
};