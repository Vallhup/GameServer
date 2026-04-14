#pragma once

#include "IAICombatActionPolicy.h"

// Imp-only attack selection policy.
//
// Selection rules:
//   - If attack cooldown is not ready, shouldAttack = false.
//   - melee2 is excluded from AI picks.
//   - melee1 is kept as a low-frequency fast poke.
//   - melee3 ~ melee5 are the main attacks, biased away from immediate repeats.
class ImpCombatActionPolicy final : public IAICombatActionPolicy {
public:
	CombatActionSelection SelectAction(const AIContext& ctx) const override;
};
