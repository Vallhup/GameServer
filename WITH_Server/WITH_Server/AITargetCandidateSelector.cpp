#include "pch.h"
#include "AITargetCandidateSelector.h"

AITargetCandidate AITargetCandidateSelector::Select(
	const AITargetSelectionSnapshot& snapshot) noexcept
{
	if (!snapshot.foundAny)
		return {};

	if (snapshot.forceRetarget)
	{
		return snapshot.hasForced
			? snapshot.forced
			: snapshot.best;
	}

	if (snapshot.alternatePlayerRetargetRequested)
	{
		if (snapshot.hasAlternatePlayer)
			return snapshot.alternatePlayer;

		return snapshot.hasCurrent
			? snapshot.current
			: snapshot.best;
	}

	if (!snapshot.hasCurrent)
		return snapshot.best;

	if (snapshot.best.entity != snapshot.current.entity &&
		snapshot.best.score >
			snapshot.current.score + snapshot.switchScoreMargin)
	{
		return snapshot.best;
	}

	return snapshot.current;
}
