#pragma once

#include "System.h"
#include "SystemMetaStorage.h"

struct AnimationPlaybackStateComp;

class ResolveAnimationPlaybackSystem final : public System {
	static const StaticSystemMetaStorage<5, 0, 1> kMetaStorage;

public:
    void Execute(SystemContext& ctx) override;
    const SystemMeta& Meta() const override { return kMetaStorage.meta; }

private:
	static bool RequiresAnimationDirty(
		const AnimationPlaybackStateComp& previousState,
		const AnimationPlaybackStateComp& nextState) noexcept;
};
