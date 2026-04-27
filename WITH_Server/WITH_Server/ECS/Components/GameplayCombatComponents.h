#pragma once

#include "GameplayActionComponents.h"

enum class SkeletalCombatColliderRoleMask : uint8_t
{
	None = 0,
	Hit = 1 << 0,
	Hurt = 1 << 1,
	Guard = 1 << 2,
	Parry = 1 << 3
};

struct SkeletalCombatCollider
{
	Capsule capsule;
	float radius{ 0.0f };
	uint8_t roleMask{ 0 };
};

struct SkeletalCombatColliderComp : Component
{
	std::vector<SkeletalCombatCollider> localColliders;
};

struct CombatColliderActivationComp : Component
{
	uint32_t boundActionInstanceId{ 0 };
	ActionId boundActionId{ ActionId::None };
	bool hasAttackWindow{ false };
	bool hasParryWindow{ false };
	bool hasGuardWindow{ false };
	bool hasInvulnerabilityWindow{ false };
};

struct CombatHitDedupStateComp : Component
{
	uint32_t boundActionInstanceId{ 0 };
	std::vector<Entity> resolvedVictims;
};

enum class CombatReactionKind : uint8_t
{
	None = 0,
	HitReaction,
	GuardBreak,
	Knockdown
};

enum class CombatResolveResultType : uint8_t
{
	Hit = 0,
	Guard,
	Parry
};

struct PendingCombatInteractionRecord
{
	Entity sourceEntity{ Entity::Null() };
	ActionId sourceActionId{ ActionId::None };
	uint32_t sourceActionInstanceId{ 0 };
	uint16_t sourceAttackWindowIndex{ 0 };
	uint16_t sourceColliderIndex{ 0 };
	uint16_t targetColliderIndex{ 0 };
	CombatResolveResultType resultType{ CombatResolveResultType::Hit };
	AttackCombatEffectDef attackEffect{};
	std::optional<GuardCombatEffectDef> guardEffect;
	std::optional<ParryCombatEffectDef> parryEffect;
	float maxKnockbackDistance{ 0.0f };
	float maxHitStopSec{ 0.0f };
};

struct PendingCombatResultComp : Component
{
	std::vector<PendingCombatInteractionRecord> receivedInteractions;
	CombatReactionKind reactionKind{ CombatReactionKind::None };
	Entity reactionSource{ Entity::Null() };
	bool guardSucceededThisFrame{ false };
	bool parrySucceededThisFrame{ false };
	bool wasHitThisFrame{ false };
	bool parriedByAnyVictimThisFrame{ false };
	bool hitAnyVictimThisFrame{ false };
	std::optional<BuffId> pendingParryBuffId;
};

struct CombatStatStateComp : Component
{
	int32_t currentHp{ 0 };
	int32_t maxHp{ 0 };
	int32_t currentStamina{ 0 };
	int32_t maxStamina{ 0 };
	int32_t currentPoise{ 0 };
	int32_t maxPoise{ 0 };
	int32_t attackPower{ 0 };
	int32_t defense{ 0 };
	float attackSpeed{ 1.0f };
	float moveSpeed{ 2.5f };
};

struct ActiveBuffRuntimeEntry
{
	BuffId buffId{ BuffId::None };
	float remainingDurationSec{ 0.0f };
	uint32_t stackCount{ 0 };
	uint64_t appliedOrder{ 0 };
};

struct BuffRuntimeStateComp : Component
{
	std::vector<ActiveBuffRuntimeEntry> activeBuffs;
};
