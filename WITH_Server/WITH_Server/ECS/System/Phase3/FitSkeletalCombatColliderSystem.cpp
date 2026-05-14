#include "pch.h"
#include "FitSkeletalCombatColliderSystem.h"

#include <span>

#include "../../../AnimationRegistry.h"
#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	float GetAnimationUnitScale(const AnimationClipDef& clip) noexcept
	{
		if (clip.units == "cm")
		{
			return 0.01f;
		}

		// TODO: If additional animation export units are introduced,
		// normalize them here instead of assuming world-space meters.
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

	void BuildCollidersFromCapsules(
		const AnimationClipDef& clip,
		std::span<const Capsule> capsules,
		float unitScale,
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
			outColliders.push_back(SkeletalCombatCollider{
				ScaleCapsule(capsule, unitScale),
				capsuleDef.radius * unitScale,
				FitSkeletalCombatColliderSystem::BuildRoleMask(clip, colliderIndex)
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

		BuildCollidersFromCapsules(
			*clip,
			std::span<const Capsule>(
				pose.localCapsules.data(),
				pose.localCapsules.size()),
			unitScale,
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
