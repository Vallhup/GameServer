#pragma once

#include "AITargetSelectionTypes.h"

class AITargetCandidateSelector final {
public:
	static AITargetCandidate Select(
		const AITargetSelectionSnapshot& snapshot) noexcept;
};
