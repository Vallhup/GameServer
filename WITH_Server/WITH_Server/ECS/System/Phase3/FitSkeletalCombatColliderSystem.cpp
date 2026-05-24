#include "pch.h"
#include "FitSkeletalCombatColliderSystem.h"

#include <algorithm>
#include <span>

#include "../../../AnimationRegistry.h"
#include "../GameplaySystemUtil.h"
#include "../../../TransformHelper.h"

using namespace GameplaySystemUtil;

namespace
{
	constexpr int kHitMotionLookAroundFrames = 2;
	constexpr float kLinkedHitMotionRatio = 0.45f;

	float GetAnimationUnitScale(const AnimationClipDef& clip) noexcept
	{
		if (clip.units == "cm")
		{
			return 0.01f;
		}

		return 1.0f;
	}

	Capsule ScaleCapsule(const Capsule& capsule, float scale) noexcept
	{
		return Capsule{
			XMFLOAT3{
				capsule.p0.x * scale,
				capsule.p0.y * scale,
				capsule.p0.z * scale,
			},
			XMFLOAT3{
				capsule.p1.x * scale,
				capsule.p1.y * scale,
				capsule.p1.z * scale,
			},
		};
	}

	bool ShouldFilterMonsterHitColliders(const AnimationClipDef& clip) noexcept
	{
		return 
			clip.skeleton == "Imp" ||
			clip.skeleton == "DemonStriker" ||
			clip.skeleton == "DemonExecutioner" ||
			clip.skeleton == "BigDemonWarrior" ||
			clip.skeleton == "Tank" ||
			clip.skeleton == "FinalBoss";
	}

	XMFLOAT3 CapsuleCenter(const Capsule& capsule) noexcept
	{
		return XMFLOAT3{
			(capsule.p0.x + capsule.p1.x) * 0.5f,
			(capsule.p0.y + capsule.p1.y) * 0.5f,
			(capsule.p0.z + capsule.p1.z) * 0.5f
		};
	}

	float CapsuleMotionScore(const Capsule& previous, const Capsule& current) noexcept
	{
		return std::max(
			TransformHelper::DistanceSq3D(previous.p0, current.p0),
			std::max(
				TransformHelper::DistanceSq3D(previous.p1, current.p1),
				TransformHelper::DistanceSq3D(CapsuleCenter(previous), CapsuleCenter(current))));
	}

	bool HasCapsuleRole(
		const AnimationClipDef& clip,
		size_t colliderIndex,
		CapsuleRole role) noexcept
	{
		if (colliderIndex >= clip.capsuleDefs.size())
		{
			return false;
		}

		const auto& roles = clip.capsuleDefs[colliderIndex].roles;
		return std::find(roles.begin(), roles.end(), role) != roles.end();
	}

	std::vector<bool> BuildActiveHitColliderMask(
		const AnimationClipDef& clip,
		uint16_t sampleFrameIndex)
	{
		if (!ShouldFilterMonsterHitColliders(clip))
		{
			return {};
		}

		std::vector<size_t> hitColliderIndices;
		hitColliderIndices.reserve(clip.capsuleDefs.size());
		for (size_t colliderIndex = 0;
			colliderIndex < clip.capsuleDefs.size();
			++colliderIndex)
		{
			if (HasCapsuleRole(clip, colliderIndex, CapsuleRole::Hit))
			{
				hitColliderIndices.push_back(colliderIndex);
			}
		}

		if (hitColliderIndices.size() <= 1 || clip.frames.size() <= 1)
		{
			return {};
		}

		const size_t frameCount = clip.frames.size();
		const size_t sampleIndex = std::min<size_t>(
			sampleFrameIndex,
			frameCount - 1);
		const size_t startFrame = sampleIndex >
			static_cast<size_t>(kHitMotionLookAroundFrames)
			? sampleIndex - static_cast<size_t>(kHitMotionLookAroundFrames)
			: 0;
		const size_t endFrame = std::min(
			frameCount - 1,
			sampleIndex + static_cast<size_t>(kHitMotionLookAroundFrames));

		std::vector<float> motionScores(clip.capsuleDefs.size(), 0.0f);
		for (size_t frameIndex = startFrame + 1;
			frameIndex <= endFrame;
			++frameIndex)
		{
			const AnimationClipFrame& previousFrame =
				clip.frames[frameIndex - 1];
			const AnimationClipFrame& currentFrame =
				clip.frames[frameIndex];
			if (previousFrame.capsules.size() != clip.capsuleDefs.size() ||
				currentFrame.capsules.size() != clip.capsuleDefs.size())
			{
				return {};
			}

			for (size_t colliderIndex : hitColliderIndices)
			{
				motionScores[colliderIndex] += CapsuleMotionScore(
					previousFrame.capsules[colliderIndex],
					currentFrame.capsules[colliderIndex]);
			}
		}

		float maxScore = 0.0f;
		for (size_t colliderIndex : hitColliderIndices)
		{
			maxScore = std::max(maxScore, motionScores[colliderIndex]);
		}
		if (maxScore <= 0.0f)
		{
			return {};
		}

		std::vector<bool> activeMask(clip.capsuleDefs.size(), false);
		const float linkedThreshold = maxScore * kLinkedHitMotionRatio;
		for (size_t colliderIndex : hitColliderIndices)
		{
			activeMask[colliderIndex] =
				motionScores[colliderIndex] >= linkedThreshold;
		}
		return activeMask;
	}

	uint8_t RemoveRole(uint8_t roleMask, SkeletalCombatColliderRoleMask role) noexcept
	{
		return static_cast<uint8_t>(
			roleMask & ~static_cast<uint8_t>(role));
	}

	void BuildCollidersFromCapsules(
		const AnimationClipDef& clip,
		std::span<const Capsule> capsules,
		float unitScale,
		const std::vector<bool>* activeHitColliderMask,
		std::vector<SkeletalCombatCollider>& outColliders)
	{
		outColliders.clear();
		outColliders.reserve(capsules.size());

		for (size_t colliderIndex = 0;
			colliderIndex < capsules.size();
			++colliderIndex)
		{
			const Capsule& capsule = capsules[colliderIndex];
			const AnimationCapsuleDef& capsuleDef = clip.capsuleDefs[colliderIndex];
			uint8_t roleMask =
				FitSkeletalCombatColliderSystem::BuildRoleMask(clip, colliderIndex);
			if (activeHitColliderMask != nullptr &&
				!activeHitColliderMask->empty() &&
				colliderIndex < activeHitColliderMask->size() &&
				!(*activeHitColliderMask)[colliderIndex])
			{
				roleMask = RemoveRole(
					roleMask,
					SkeletalCombatColliderRoleMask::Hit);
			}

			outColliders.push_back(SkeletalCombatCollider{
				ScaleCapsule(capsule, unitScale),
				capsuleDef.radius * unitScale,
				roleMask,
				capsuleDef.boneIndex
			});
		}
	}
}

const StaticSystemMetaStorage<3> FitSkeletalCombatColliderSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<FitSkeletalCombatColliderSystem>(),
		"FitSkeletalCombatColliderSystem",
		std::array<AccessSpec, 3>
	{
		ReadImmediate(ComponentRes<SampledAnimationPoseComp>()),
		WriteImmediate(ComponentRes<SkeletalCombatColliderComp>()),
		ReadImmediate(ExternalRes<AnimationRegistry>()),
	});

FitSkeletalCombatColliderSystem::FitSkeletalCombatColliderSystem(
	const AnimationRegistry* animationRegistry)
	: _animationRegistry(animationRegistry)
{
}

void FitSkeletalCombatColliderSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, pose, colliderState] :
		ctx.ecs.View<SampledAnimationPoseComp, SkeletalCombatColliderComp>())
	{
		(void)entity;
		colliderState.localColliders.clear();
		colliderState.previousFrameLocalColliders.clear();
		colliderState.hasPreviousFrameLocalColliders = false;

		if (_animationRegistry == nullptr ||
			pose.animationId == AnimationId::None)
		{
			continue;
		}

		const AnimationClipDef* clip = _animationRegistry->Find(pose.animationId);
		if (clip == nullptr ||
			clip->capsuleDefs.size() != pose.localCapsules.size())
		{
			continue;
		}

		const float unitScale = GetAnimationUnitScale(*clip);
		const std::vector<bool> activeHitColliderMask =
			BuildActiveHitColliderMask(*clip, pose.sampleFrameIndex);

		BuildCollidersFromCapsules(
			*clip,
			std::span<const Capsule>(
				pose.localCapsules.data(),
				pose.localCapsules.size()),
			unitScale,
			&activeHitColliderMask,
			colliderState.localColliders);

		if (pose.sampleFrameIndex > 0 &&
			pose.sampleFrameIndex < clip->frames.size())
		{
			const AnimationClipFrame& previousFrame =
				clip->frames[pose.sampleFrameIndex - 1];
			if (previousFrame.capsules.size() == clip->capsuleDefs.size())
			{
				BuildCollidersFromCapsules(
					*clip,
					std::span<const Capsule>(
						previousFrame.capsules.data(),
						previousFrame.capsules.size()),
					unitScale,
					&activeHitColliderMask,
					colliderState.previousFrameLocalColliders);
				colliderState.hasPreviousFrameLocalColliders = true;
			}
		}
	}
}

uint8_t FitSkeletalCombatColliderSystem::BuildRoleMask(
	const AnimationClipDef& clip,
	size_t colliderIndex)
{
	if (colliderIndex >= clip.capsuleDefs.size())
	{
		return 0;
	}

	uint8_t roleMask = 0;
	for (CapsuleRole role : clip.capsuleDefs[colliderIndex].roles)
	{
		switch (role) {
		case CapsuleRole::Hit:
		{
			roleMask |= static_cast<uint8_t>(SkeletalCombatColliderRoleMask::Hit);
			break;
		}
		case CapsuleRole::Hurt:
		{
			roleMask |= static_cast<uint8_t>(SkeletalCombatColliderRoleMask::Hurt);
			break;
		}
		case CapsuleRole::Guard:
		{
			roleMask |= static_cast<uint8_t>(SkeletalCombatColliderRoleMask::Guard);
			break;
		}
		case CapsuleRole::Parry:
		{
			roleMask |= static_cast<uint8_t>(SkeletalCombatColliderRoleMask::Parry);
			break;
		}
		case CapsuleRole::None:
		default:
			break;
		}
	}

	return roleMask;
}
