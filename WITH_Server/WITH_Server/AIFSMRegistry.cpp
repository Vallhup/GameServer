#include "pch.h"
#include "AIFSMRegistry.h"

#include "NormalAIIdleState.h"
#include "NormalAIChaseState.h"
#include "NormalAICombatState.h"
#include "NormalAISearchState.h"
#include "NormalAIReactState.h"

#include "NormalAIMovementPolicy.h"

/* --------------[ AIStateRegistry ]-------------- */

AIStateRegistry::AIStateRegistry(AIArchetype type)
{
	_states[static_cast<size_t>(AIStateType::Idle)] = std::make_unique<NormalAIIdleState>();
	_states[static_cast<size_t>(AIStateType::Chase)] = std::make_unique<NormalAIChaseState>();
	_states[static_cast<size_t>(AIStateType::Combat)] = std::make_unique<NormalAICombatState>();
	_states[static_cast<size_t>(AIStateType::Search)] = std::make_unique<NormalAISearchState>();
	_states[static_cast<size_t>(AIStateType::React)] = std::make_unique<NormalAIReactState>();
}

const IAIState* AIStateRegistry::TryGetState(AIStateType type) const
{
	const size_t index = static_cast<size_t>(type);

	if (index >= _states.size())
		return nullptr;

	return _states[index].get();
}

/* --------------[ AIFSMBundle ]-------------- */

AIFSMBundle::AIFSMBundle(AIArchetype type) : aiType(type), stateRegistry(AIStateRegistry(type))
{
	switch (type) {
	case AIArchetype::NormalMonster:
	{
		movementPolicy = std::make_unique<NormalAIMovementPolicy>();
		break;
	}
	default:
	{
		movementPolicy = nullptr;
		break;
	}
	}
}


/* --------------[ AIFSMRegistry ]-------------- */

AIFSMRegistry::AIFSMRegistry() : _normal(AIFSMBundle(AIArchetype::NormalMonster))
{
	
}

const AIFSMBundle* AIFSMRegistry::TryGetBundle(AIArchetype type) const
{
	switch (type) {
	case AIArchetype::NormalMonster:
	{
		return &_normal;
	}
	default:
	{
		return nullptr;
	}
	}
}

