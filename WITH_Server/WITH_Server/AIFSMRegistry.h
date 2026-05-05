#pragma once

#include <array>
#include <memory>
#include <span>
#include <unordered_map>

#include "AIBehaviorDef.h"
#include "IAIState.h"
#include "IAIMovementPolicy.h"
#include "IAICombatActionPolicy.h"
#include "IAIReactionPolicy.h"
#include "IAISpecialActionPolicy.h"

class AIStateRegistry final {
	static constexpr size_t kAIStateCount = static_cast<size_t>(AIStateType::Count);

public:
	AIStateRegistry();

	AIStateRegistry(const AIStateRegistry&)            = delete;
	AIStateRegistry& operator=(const AIStateRegistry&) = delete;

	AIStateRegistry(AIStateRegistry&&)            = default;
	AIStateRegistry& operator=(AIStateRegistry&&) = default;

	const IAIState* TryGetState(AIStateType type) const;

private:
	std::array<std::unique_ptr<IAIState>, kAIStateCount> _states;
};

struct AIFSMBundle
{
	AIArchetype      aiType;
	AIStateRegistry  stateRegistry;

	AIFSMBundle() = delete;
	AIFSMBundle(AIArchetype type) : aiType(type) {}

	AIFSMBundle(const AIFSMBundle&)            = delete;
	AIFSMBundle& operator=(const AIFSMBundle&) = delete;

	AIFSMBundle(AIFSMBundle&&)            = default;
	AIFSMBundle& operator=(AIFSMBundle&&) = default;
};

struct AIBehaviorBundle
{
	const AIBehaviorProfileDef* profile{ nullptr };

	std::unique_ptr<IAIMovementPolicy>			movementPolicy;
	std::unique_ptr<IAICombatActionPolicy>		combatActionPolicy;
	std::unique_ptr<IAIReactionPolicy>			reactionPolicy;
	std::unique_ptr<IAISpecialActionPolicy>     specialActionPolicy;

	AIBehaviorBundle() = delete;
	explicit AIBehaviorBundle(const AIBehaviorProfileDef& profileDef);

	AIBehaviorBundle(const AIBehaviorBundle&)            = delete;
	AIBehaviorBundle& operator=(const AIBehaviorBundle&) = delete;

	AIBehaviorBundle(AIBehaviorBundle&&)            = default;
	AIBehaviorBundle& operator=(AIBehaviorBundle&&) = default;
};

class AIFSMRegistry final {
public:
	AIFSMRegistry();
	explicit AIFSMRegistry(std::span<const AIBehaviorProfileDef> profiles);

	AIFSMRegistry(const AIFSMRegistry&)            = delete;
	AIFSMRegistry& operator=(const AIFSMRegistry&) = delete;

	AIFSMRegistry(AIFSMRegistry&&)            = default;
	AIFSMRegistry& operator=(AIFSMRegistry&&) = default;

	const AIFSMBundle* TryGetBundle(AIArchetype type) const;

	const AIBehaviorBundle* TryGetBehavior(
		AIBehaviorProfileId profileId) const;

	static bool IsArchetypeSupported(AIArchetype type) noexcept;

	static bool IsBehaviorProfileSupported(
		const AIBehaviorProfileDef& profile) noexcept;

	static bool IsBehaviorSupported(
		AIBehaviorProfileId profileId) noexcept;

private:
	void Build(std::span<const AIBehaviorProfileDef> profiles);

	std::unordered_map<AIArchetype, AIFSMBundle, DefRegistryIdHash<AIArchetype>> _bundles;
	std::unordered_map<AIBehaviorProfileId, AIBehaviorBundle> _behaviors;
};
