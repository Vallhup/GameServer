#pragma once

#include <algorithm>
#include <cmath>

namespace BodyCollisionSlide
{
	struct XZDelta
	{
		float x{ 0.0f };
		float z{ 0.0f };
	};

	inline XZDelta ProjectOntoSurfaceTangent(
		float motionX,
		float motionZ,
		float normalX,
		float normalZ,
		bool preserveMotionLength = true,
		float epsilon = 1.0e-5f) noexcept
	{
		const float normalLengthSq =
			normalX * normalX + normalZ * normalZ;
		const float motionLengthSq =
			motionX * motionX + motionZ * motionZ;
		if (normalLengthSq <= epsilon * epsilon ||
			motionLengthSq <= epsilon * epsilon)
		{
			return {};
		}

		const float inverseNormalLength =
			1.0f / std::sqrt(normalLengthSq);
		const float normalizedNormalX = normalX * inverseNormalLength;
		const float normalizedNormalZ = normalZ * inverseNormalLength;
		const float normalMotion =
			motionX * normalizedNormalX + motionZ * normalizedNormalZ;

		XZDelta tangent{
			.x = motionX - normalizedNormalX * normalMotion,
			.z = motionZ - normalizedNormalZ * normalMotion
		};

		if (!preserveMotionLength)
		{
			return tangent;
		}

		const float tangentLengthSq =
			tangent.x * tangent.x + tangent.z * tangent.z;
		if (tangentLengthSq <= epsilon * epsilon)
		{
			return {};
		}

		const float lengthScale =
			std::sqrt(motionLengthSq / tangentLengthSq);
		tangent.x *= lengthScale;
		tangent.z *= lengthScale;
		return tangent;
	}

	inline XZDelta ComputeSpeedPreservingAdjustment(
		float desiredMotionX,
		float desiredMotionZ,
		float resolvedMotionX,
		float resolvedMotionZ,
		float surfaceNormalX,
		float surfaceNormalZ,
		float epsilon = 1.0e-5f) noexcept
	{
		const float normalLengthSq =
			surfaceNormalX * surfaceNormalX +
			surfaceNormalZ * surfaceNormalZ;
		if (normalLengthSq <= epsilon * epsilon)
		{
			return {};
		}

		const float inverseNormalLength =
			1.0f / std::sqrt(normalLengthSq);
		const float normalX = surfaceNormalX * inverseNormalLength;
		const float normalZ = surfaceNormalZ * inverseNormalLength;
		const float desiredNormalMotion =
			desiredMotionX * normalX + desiredMotionZ * normalZ;
		if (desiredNormalMotion >= -epsilon)
		{
			return {};
		}

		const float resolvedNormalMotion =
			resolvedMotionX * normalX + resolvedMotionZ * normalZ;
		const float hitRatio = std::clamp(
			resolvedNormalMotion / desiredNormalMotion,
			0.0f,
			1.0f);
		const float remainingRatio = 1.0f - hitRatio;

		const XZDelta preHitTangent = ProjectOntoSurfaceTangent(
			desiredMotionX * hitRatio,
			desiredMotionZ * hitRatio,
			surfaceNormalX,
			surfaceNormalZ,
			false,
			epsilon);
		const XZDelta remainingSlide = ProjectOntoSurfaceTangent(
			desiredMotionX * remainingRatio,
			desiredMotionZ * remainingRatio,
			surfaceNormalX,
			surfaceNormalZ,
			true,
			epsilon);
		const XZDelta resolvedTangent = ProjectOntoSurfaceTangent(
			resolvedMotionX,
			resolvedMotionZ,
			surfaceNormalX,
			surfaceNormalZ,
			false,
			epsilon);

		return {
			.x = preHitTangent.x + remainingSlide.x - resolvedTangent.x,
			.z = preHitTangent.z + remainingSlide.z - resolvedTangent.z
		};
	}
}
