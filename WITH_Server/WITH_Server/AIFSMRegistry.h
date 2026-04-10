#pragma once

#include "EntityId.h"
#include "IAIState.h"
#include "IAIMovementPolicy.h"
#include "IAICombatActionPolicy.h"
#include "IAIReactionPolicy.h"

class AIStateRegistry final
{
public:
	AIStateRegistry() = delete;
	AIStateRegistry(AIArchetype type);

	AIStateRegistry(const AIStateRegistry&)            = delete;
	AIStateRegistry& operator=(const AIStateRegistry&) = delete;

	AIStateRegistry(AIStateRegistry&&)            = default;
	AIStateRegistry& operator=(AIStateRegistry&&) = default;

	const IAIState* TryGetState(AIStateType type) const;

private:
	std::array<std::unique_ptr<IAIState>, 5> _states;
};

struct AIFSMBundle
{
	AIArchetype      aiType;
	AIStateRegistry  stateRegistry;

	std::unique_ptr<IAIMovementPolicy>     movementPolicy;
	std::unique_ptr<IAICombatActionPolicy> combatActionPolicy;
	std::unique_ptr<IAIReactionPolicy>     reactionPolicy;

	AIFSMBundle() = delete;
	AIFSMBundle(AIArchetype type);

	AIFSMBundle(const AIFSMBundle&)            = delete;
	AIFSMBundle& operator=(const AIFSMBundle&) = delete;

	AIFSMBundle(AIFSMBundle&&)            = default;
	AIFSMBundle& operator=(AIFSMBundle&&) = default;
};

class AIFSMRegistry final
{
public:
	AIFSMRegistry();

	AIFSMRegistry(const AIFSMRegistry&)            = delete;
	AIFSMRegistry& operator=(const AIFSMRegistry&) = delete;

	AIFSMRegistry(AIFSMRegistry&&)            = default;
	AIFSMRegistry& operator=(AIFSMRegistry&&) = default;

	const AIFSMBundle* TryGetBundle(AIArchetype type) const;
	static bool        IsArchetypeSupported(AIArchetype type) noexcept;

private:
	AIFSMBundle _normal;
};
