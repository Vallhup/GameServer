#pragma once

#include "IAIState.h"

class AIStateRegistry final {
public:
	AIStateRegistry();

	const IAIState* TryGetState(AIStateType type) const
	{
		const size_t index = static_cast<size_t>(type);

		if (index >= _states.size())
			return nullptr;

		return _states[index].get();
	}

private:
	std::array<std::unique_ptr<IAIState>, 5> _states;
};

struct AIFSMBundle
{
	AIStateRegistry stateRegistry;
	std::unique_ptr<IMovementPolicy> movementPolicy;
	// TODO : 추후 늘어날 수 있음
};

class AIFSMRegistry final {
public:
	AIFSMRegistry();

	const AIFSMBundle* TryGetBundle(EntityType type) const;

private:
	AIFSMBundle _normal;

	// TODO : 추후 늘어날 수 있음
};

