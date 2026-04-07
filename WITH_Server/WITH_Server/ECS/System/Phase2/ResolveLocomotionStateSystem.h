#pragma once

#include "System.h"

class ResolveLocomotionStateSystem final : public System {
public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override;

private:
	static const SystemMeta kMeta;
};
