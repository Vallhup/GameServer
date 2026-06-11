#pragma once

#include "AIBehaviorDef.h"
#include "IAICombatActionPolicy.h"

class AICombatActionRuntimeUpdater final {
public:
	static void Commit(
		AIContext& ctx,
		const CombatActionSelection& selection);

private:
	static void ApplyAction(
		AIContext& ctx,
		const AIActionDef& action,
		size_t actionIndex);
};
