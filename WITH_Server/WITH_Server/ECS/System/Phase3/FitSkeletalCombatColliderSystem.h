#pragma once

#include <cstddef>

#include "System.h"

struct AnimationClipDef;
class AnimationRegistry;

class FitSkeletalCombatColliderSystem final : public System {
public:
	explicit FitSkeletalCombatColliderSystem(
		const AnimationRegistry* animationRegistry);

	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override;

private:
	static uint8_t BuildRoleMask(const AnimationClipDef& clip, size_t colliderIndex);

	static const SystemMeta kMeta;
	const AnimationRegistry* _animationRegistry{ nullptr };
};
