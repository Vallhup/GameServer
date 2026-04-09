#pragma once

#include "System.h"
#include "../../GameplayRuntimeComponents.h"

class ResolveAnimationPlaybackSystem final : public System {
public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override;

private:
	static bool RequiresAnimationDirty(
		const AnimationPlaybackStateComp& previousState,
		const AnimationPlaybackStateComp& nextState) noexcept;

	static const SystemMeta kMeta;
};
