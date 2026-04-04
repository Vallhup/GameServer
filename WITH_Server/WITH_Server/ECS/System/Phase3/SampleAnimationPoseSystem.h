#pragma once

#include "System.h"

class AnimationRegistry;

class SampleAnimationPoseSystem final : public System {
public:
	explicit SampleAnimationPoseSystem(const AnimationRegistry* animationRegistry);

	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override;

private:
	static const SystemMeta kMeta;
	const AnimationRegistry* _animationRegistry{ nullptr };
};
