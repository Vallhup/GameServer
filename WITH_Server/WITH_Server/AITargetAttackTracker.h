#pragma once

#include "Entity.h"

struct AIContext;
struct AIBlackboardComp;

class AITargetAttackTracker final {
public:
	static void RecordCommittedAttack(
		AIContext& ctx,
		Entity target) noexcept;

	static void ResetForTarget(
		AIBlackboardComp& blackboard,
		Entity target) noexcept;
};
