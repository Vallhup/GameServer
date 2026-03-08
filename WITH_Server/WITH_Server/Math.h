#pragma once

#include <DirectXMath.h>
#include "Transform.h"

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

	inline XMVECTOR ForwardFromYaw(float yaw)
	{
		return XMVectorSet(-sin(yaw), 0.0f, -cos(yaw), 0.0f);
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

	inline bool SafeNormalize3(XMVECTOR v, XMVECTOR& out)
	{
		XMVECTOR lenSq = XMVector3LengthSq(v);
		float ls = XMVectorGetX(lenSq);

		if (!(ls > 1e-12f)) return false;

		out = XMVector3Normalize(v);
		return true;
	}

	inline bool IsInFront90_XZ(const Transform& attackerTr,
		const Transform& victimTr)
	{
		XMVECTOR aPos = XMLoadFloat3(&attackerTr.position);
		XMVECTOR vPos = XMLoadFloat3(&victimTr.position);

		// dir = aPos - vPos = victim -> attacker
		XMVECTOR dir = XMVectorSubtract(aPos, vPos);
		dir = XMVectorSetY(dir, 0.0f);

		// 위치가 너무 가까우면 판정에서 제외
		float lenSq = XMVectorGetX(XMVector3LengthSq(dir));
		if (lenSq < 1e-6f) return false;

		dir = XMVector3Normalize(dir);

		XMVECTOR vRot = XMLoadFloat4(&victimTr.rotation);

		const XMVECTOR baseForward = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
		
		// vForward = victim의 전방 벡터
		XMVECTOR vForward = XMVector3Rotate(baseForward, vRot);
		vForward = XMVectorSetY(vForward, 0.0f);
		vForward = XMVector3Normalize(vForward);

		const float cos45 = 0.70710678f;
		const float d = XMVectorGetX(XMVector3Dot(vForward, dir));
		return d >= cos45;
	}

	inline float WrapPi(float a)
	{
		while (a > XM_PI) a -= XM_2PI;
		while (a <= -XM_PI) a += XM_2PI;
		return a;
	}

	inline float AngleDelta(float from, float to)
	{
		return WrapPi(to - from);
	}
}