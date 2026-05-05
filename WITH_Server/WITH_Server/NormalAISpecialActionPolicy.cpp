#include "pch.h"
#include "NormalAISpecialActionPolicy.h"

void NormalAISpecialActionPolicy::TickRuntime(AIContext& ctx, const double dtSec) const
{
}

bool NormalAISpecialActionPolicy::TryIssuePreFSMAction(AIContext& ctx) const
{
	return false;
}
