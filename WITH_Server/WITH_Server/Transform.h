#pragma once

#include <DirectXMath.h>
#include "Component.h"

struct Transform : public Component {
	DirectX::XMFLOAT3 position{ 80.0f, 5.0f, 80.0f };
	DirectX::XMFLOAT4 rotation{ 0, 0, 0, 1 };
	DirectX::XMFLOAT3 scale{ 1, 1, 1 };
};