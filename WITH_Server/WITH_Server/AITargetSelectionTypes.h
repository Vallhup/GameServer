#pragma once

#include "Entity.h"

struct AITargetCandidate
{
	Entity entity{ Entity::Null() };

	bool isCurrentTarget{ false };
	bool isLastAttacker{ false };

	double distSq{ 0.0 };

	bool inSightRange{ false };
	bool inAttackRange{ false };

	double forwardDot{ 0.0 };
	bool inFront{ false };

	bool visible{ false };
	double score{ 0.0 };
};

struct AITargetSelectionSnapshot
{
	AITargetCandidate best;
	AITargetCandidate current;
	AITargetCandidate forced;
	AITargetCandidate alternatePlayer;

	bool foundAny{ false };
	bool hasCurrent{ false };
	bool hasForced{ false };
	bool hasAlternatePlayer{ false };

	bool forceRetarget{ false };
	bool alternatePlayerRetargetRequested{ false };
	double switchScoreMargin{ 0.0 };
};
