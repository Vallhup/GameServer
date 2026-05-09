#pragma once

#include "System.h"
#include "SystemMetaStorage.h"

class AnimationRegistry;

class SampleAnimationPoseSystem final : public System {
	static const StaticSystemMetaStorage<3> kMetaStorage;

public:
	explicit SampleAnimationPoseSystem(const AnimationRegistry* animationRegistry);

	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }

private:
	const AnimationRegistry* _animationRegistry{ nullptr };
};
