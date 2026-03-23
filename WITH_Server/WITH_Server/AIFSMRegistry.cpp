#include "pch.h"
#include "AIFSMRegistry.h"

#include "NormalAIIdleState.h"
#include "NormalAIChaseState.h"
#include "NormalAICombatState.h"
#include "NormalAISearchState.h"
#include "NormalAIReactState.h"

#include "NormalAIMovementPolicy.h"

AIStateRegistry::AIStateRegistry()
{
	_states[static_cast<size_t>(AIStateType::Idle)] = std::make_unique<NormalAIIdleState>();
	_states[static_cast<size_t>(AIStateType::Chase)] = std::make_unique<NormalAIChaseState>();
	_states[static_cast<size_t>(AIStateType::Combat)] = std::make_unique<NormalAICombatState>();
	_states[static_cast<size_t>(AIStateType::Search)] = std::make_unique<NormalAISearchState>();
	_states[static_cast<size_t>(AIStateType::React)] = std::make_unique<NormalAIReactState>();
}

AIFSMRegistry::AIFSMRegistry()
{
	_normal.movementPolicy = std::make_unique<NormalAIMovementPolicy>();
}

const AIFSMBundle* AIFSMRegistry::TryGetBundle(EntityType type) const
{
	switch (type) {
	case EntityType::Final_Boss:
	{
		return &_normal;
	}
	default:
	{
		return nullptr;
	}
	}
}
