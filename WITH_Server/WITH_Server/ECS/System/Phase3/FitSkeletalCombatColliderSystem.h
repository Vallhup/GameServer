#pragma once

#include "System.h"
#include "SystemMetaStorage.h"

struct AnimationClipDef;
class AnimationRegistry;

class FitSkeletalCombatColliderSystem final : public System {
	static const StaticSystemMetaStorage<3> kMetaStorage;

public:
	explicit FitSkeletalCombatColliderSystem(
		const AnimationRegistry* animationRegistry);

	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }

private:
	static uint8_t BuildRoleMask(const AnimationClipDef& clip, size_t colliderIndex);

	const AnimationRegistry* _animationRegistry{ nullptr };
};
