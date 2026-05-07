#include "pch.h"
#include "SessionFlowTransitionTable.h"

TransitionResult SessionFlowTransitionTable::Dispatch(
	SessionFlowContext& ctx,
	SessionStateId stateId,
	const ISessionCommand& command) const
{
	const auto it = _transitions.find(Key{ stateId, command.Id() });
	if (it == _transitions.end())
	{
		return TransitionResult::Invalid();
	}

	return it->second(ctx, command);
}

bool SessionFlowTransitionTable::Contains(
	SessionStateId stateId,
	SessionCommandId commandId) const
{
	return _transitions.find(Key{ stateId, commandId }) != _transitions.end();
}

void SessionFlowTransitionTable::Clear() noexcept
{
	_transitions.clear();
}
