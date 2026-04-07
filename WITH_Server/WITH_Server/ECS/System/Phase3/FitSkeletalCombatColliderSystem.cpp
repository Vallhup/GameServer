#include "pch.h"
#include "FitSkeletalCombatColliderSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

const SystemMeta FitSkeletalCombatColliderSystem::kMeta =
	MakeSystemMeta<FitSkeletalCombatColliderSystem>(
		"FitSkeletalCombatColliderSystem");

void FitSkeletalCombatColliderSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, pose, colliderState] :
		ctx.ecs.View<SampledAnimationPoseComp, SkeletalCombatColliderComp>())
	{
		(void)entity;
		colliderState.localColliders.clear();
		colliderState.localColliders.reserve(pose.localCapsules.size());

		for (const Capsule& capsule : pose.localCapsules)
		{
			colliderState.localColliders.push_back(SkeletalCombatCollider{
				capsule,
				kDefaultColliderRadius,
				static_cast<uint8_t>(SkeletalCombatColliderRoleMask::Hit) |
					static_cast<uint8_t>(SkeletalCombatColliderRoleMask::Hurt)
			});
		}
	}
}

const SystemMeta& FitSkeletalCombatColliderSystem::Meta() const
{
	return kMeta;
}
