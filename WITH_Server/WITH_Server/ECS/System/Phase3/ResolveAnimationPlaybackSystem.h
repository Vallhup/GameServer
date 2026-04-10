#pragma once

#include "System.h"
#include "SystemMetaStorage.h"
#include "../../GameplayRuntimeComponents.h"

class ResolveAnimationPlaybackSystem final : public System
{
public:
    void Execute(SystemContext& ctx) override;
    const SystemMeta& Meta() const override { return kMetaStorage.meta; }

private:
	static bool RequiresAnimationDirty(
		const AnimationPlaybackStateComp& previousState,
		const AnimationPlaybackStateComp& nextState) noexcept;

    static const StaticSystemMetaStorage<5, 0, 1> kMetaStorage;
};
