#pragma once

#include <cmath>
#include <DirectXMath.h>
#include "ECS/GameplayRuntimeComponents.h"

using namespace DirectX;

namespace TransformHelper
{
	// -------------------------------------------------------------------------
	// 행렬 / 쿼터니언
	// -------------------------------------------------------------------------

	inline XMMATRIX ToMatrix(const WorldTransformComp& t)
	{
		XMVECTOR S = XMLoadFloat3(&t.scale);
		XMVECTOR R = XMLoadFloat4(&t.rotation);
		XMVECTOR P = XMLoadFloat3(&t.position);
		return XMMatrixAffineTransformation(S, XMVectorZero(), R, P);
	}

	inline XMVECTOR Forward(const WorldTransformComp& t)
	{
		return XMVector3Normalize(
			XMVector3TransformNormal(
				XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f),
				ToMatrix(t)
			)
		);
	}

	inline XMVECTOR ForwardFromYaw(float yaw)
	{
		return XMVectorSet(-std::sin(yaw), 0.0f, -std::cos(yaw), 0.0f);
	}

	inline float QuaternionToYaw(const XMFLOAT4& quat)
	{
		XMVECTOR q = XMLoadFloat4(&quat);
		XMVECTOR forward = XMVector3Rotate(XMVectorSet(0, 0, 1, 0), q);
		return std::atan2(XMVectorGetX(forward), XMVectorGetZ(forward));
	}

	inline bool SafeNormalize3(XMVECTOR v, XMVECTOR& out)
	{
		float ls = XMVectorGetX(XMVector3LengthSq(v));
		if (!(ls > 1e-12f)) return false;
		out = XMVector3Normalize(v);
		return true;
	}

	inline bool IsInFront90_XZ(const WorldTransformComp& attackerTr, const WorldTransformComp& victimTr)
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

	// XZ 평면 거리 제곱 (double 정밀도)
	inline double DistanceSq(const WorldTransformComp& t1, const WorldTransformComp& t2)
	{
		const double dx = t1.position.x - t2.position.x;
		const double dz = t1.position.z - t2.position.z;
		return dx * dx + dz * dz;
	}

	inline double DistanceSq(const XMFLOAT3& t1, const XMFLOAT3& t2)
	{
		const double dx = t1.x - t2.x;
		const double dz = t1.z - t2.z;
		return dx * dx + dz * dz;
	}

	inline XMVECTOR Direction(const WorldTransformComp& from, const WorldTransformComp& to)
	{
		const XMVECTOR vFrom = XMLoadFloat3(&from.position);
		const XMVECTOR vTo = XMLoadFloat3(&to.position);

		XMVECTOR dir = XMVectorSubtract(vTo, vFrom);
		if (SafeNormalize3(dir, dir))
			return dir;

		return XMVectorZero();
	}

	inline XMVECTOR Direction(const XMFLOAT3& from, const XMFLOAT3& to)
	{
		const XMVECTOR vFrom = XMLoadFloat3(&from);
		const XMVECTOR vTo = XMLoadFloat3(&to);

		XMVECTOR dir = XMVectorSubtract(vTo, vFrom);
		if (SafeNormalize3(dir, dir))
			return dir;

		return XMVectorZero();
	}

	// XMVECTOR 내적 (double 반환)
	inline double Dot(const XMVECTOR v1, const XMVECTOR v2)
	{
		return static_cast<double>(XMVectorGetX(XMVector3Dot(v1, v2)));
	}

	// -------------------------------------------------------------------------
	// XMFLOAT3 직접 산술 연산 (load/store 없는 캡슐·충돌 수학용)
	// -------------------------------------------------------------------------

	inline XMFLOAT3 Add(const XMFLOAT3& lhs, const XMFLOAT3& rhs) noexcept
	{
		return XMFLOAT3{ lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z };
	}

	inline XMFLOAT3 Subtract(const XMFLOAT3& lhs, const XMFLOAT3& rhs) noexcept
	{
		return XMFLOAT3{ lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z };
	}

	inline XMFLOAT3 Scale(const XMFLOAT3& v, float s) noexcept
	{
		return XMFLOAT3{ v.x * s, v.y * s, v.z * s };
	}

	// XMFLOAT3 내적 (float 반환, XMVECTOR 변환 없음)
	inline float DotF(const XMFLOAT3& lhs, const XMFLOAT3& rhs) noexcept
	{
		return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
	}

	// XMFLOAT3 안전한 정규화 (인플레이스, 길이 0이면 zero벡터 반환)
	inline bool TryNormalize(XMFLOAT3& v) noexcept
	{
		const float lengthSq = DotF(v, v);
		if (lengthSq <= 1.0e-6f)
		{
			v = XMFLOAT3{ 0.0f, 0.0f, 0.0f };
			return false;
		}

		const float invLength = 1.0f / std::sqrt(lengthSq);
		v.x *= invLength;
		v.y *= invLength;
		v.z *= invLength;
		return true;
	}

	// 3D 거리 제곱 (float, 전체 축)
	inline float DistanceSq3D(const XMFLOAT3& a, const XMFLOAT3& b) noexcept
	{
		const float dx = a.x - b.x;
		const float dy = a.y - b.y;
		const float dz = a.z - b.z;
		return dx * dx + dy * dy + dz * dz;
	}

	// -------------------------------------------------------------------------
	// XZ 평면 스칼라 수학
	// -------------------------------------------------------------------------

	inline float LengthXZ(float x, float z) noexcept
	{
		return std::sqrt(x * x + z * z);
	}

	// XZ 평면 정규화 (인플레이스). 길이가 임계값 이하면 zero 후 false 반환
	inline bool NormalizeXZ(float& x, float& z) noexcept
	{
		const float length = LengthXZ(x, z);
		if (length <= 1.0e-4f)
		{
			x = 0.0f;
			z = 0.0f;
			return false;
		}

		x /= length;
		z /= length;
		return true;
	}

	// XZ 방향 벡터를 Yaw 각도(라디안)로 변환
	inline float DirToYaw(float x, float z, float fallbackYaw) noexcept
	{
		if (LengthXZ(x, z) <= 1.0e-4f)
			return fallbackYaw;

		return std::atan2(-x, -z);
	}

	// 최대 회전량을 제한하며 목표 Yaw로 보간
	inline float ClampYawStep(float currentYaw, float targetYaw, float maxStep) noexcept
	{
		const float delta = WrapPi(targetYaw - currentYaw);
		if (std::abs(delta) <= maxStep)
			return targetYaw;

		return WrapPi(currentYaw + std::copysign(maxStep, delta));
	}

	// -------------------------------------------------------------------------
	// 트랜스폼 유틸리티
	// -------------------------------------------------------------------------

	// 스케일 컴포넌트 중 최댓값 (반경 스케일 계산용)
	inline float MaxScaleComponent(const WorldTransformComp& t) noexcept
	{
		return std::max(t.scale.x, std::max(t.scale.y, t.scale.z));
	}

	// 로컬 좌표를 월드 행렬로 변환
	inline XMFLOAT3 TransformPoint(
		const XMMATRIX& worldMatrix,
		const XMFLOAT3& point) noexcept
	{
		XMFLOAT3 result{};
		XMStoreFloat3(
			&result,
			XMVector3TransformCoord(XMLoadFloat3(&point), worldMatrix));
		return result;
	}

	// Yaw 델타 회전을 쿼터니언에 적용 (deltaYaw가 임계값 이하면 무시)
	inline void ApplyYawRotation(
		WorldTransformComp& transform,
		float deltaYaw) noexcept
	{
		if (std::abs(deltaYaw) <= 1.0e-4f)
			return;

		XMVECTOR curRot = XMLoadFloat4(&transform.rotation);
		XMVECTOR deltaRot = XMQuaternionRotationAxis(
			XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),
			deltaYaw);
		XMVECTOR nextRot = XMQuaternionNormalize(
			XMQuaternionMultiply(deltaRot, curRot));
		XMStoreFloat4(&transform.rotation, nextRot);
	}
}