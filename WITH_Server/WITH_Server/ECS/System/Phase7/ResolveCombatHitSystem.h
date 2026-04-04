#pragma once

#include "System.h"

class ResolveCombatHitSystem final : public System {
public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override;

private:
	static const SystemMeta kMeta;
};
