#pragma once

#include <DirectXMath.h>
#include "Component.h"

using namespace DirectX;

namespace TransformHelper
{
	inline XMMATRIX ToMatrix(const Transform& t)
	{
		XMVECTOR S = XMLoadFloat3(&t.scale);
		XMVECTOR R = XMLoadFloat4(&t.rotation);
		XMVECTOR P = XMLoadFloat3(&t.position);
		return XMMatrixAffineTransformation(S, XMVectorZero(), R, P);
	}

	inline XMVECTOR Forward(const Transform& t)
	{
		return XMVector3Normalize(
			XMVector3TransformNormal(
				XMVectorSet(0, 0, -1, 0),
				ToMatrix(t)
			)
		);
	}

	inline float QuaternionToYaw(const XMFLOAT4& quat)
	{
		XMVECTOR q = XMLoadFloat4(&quat);

		XMVECTOR forward = XMVectorSet(0, 0, 1, 0);
		forward = XMVector3Rotate(forward, q);

		float fx = XMVectorGetX(forward);
		float fz = XMVectorGetZ(forward);

		return std::atan2(fx, fz);
	}
}