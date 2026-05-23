#pragma once

#include "GameplayAbilityComponents.h"

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
	uint16_t boneIndex{ 0 };
};

struct SkeletalCombatColliderComp : Component
{
	std::vector<SkeletalCombatCollider> localColliders;
	std::vector<SkeletalCombatCollider> previousFrameLocalColliders;
	bool hasPreviousFrameLocalColliders{ false };
};

struct CombatColliderActivationComp : Component
{
	uint32_t boundAbilityInstanceId{ 0 };
	AbilityId boundAbilityId{ InvalidAbilityId };
	bool hasAttackWindow{ false };
	bool hasParryWindow{ false };
	bool hasGuardWindow{ false };
	bool hasInvulnerabilityWindow{ false };
};

inline constexpr uint16_t InvalidCombatColliderIndex = 0xFFFF;

struct CombatHitResolvedVictim
{
	Entity victim{ Entity::Null() };
	uint16_t attackWindowIndex{ InvalidCombatColliderIndex };
};

struct CombatHitDedupStateComp : Component
{
	uint32_t boundAbilityInstanceId{ 0 };
	std::vector<CombatHitResolvedVictim> resolvedVictims;
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

enum class CombatHitSourceKind : uint8_t
{
	SkeletalCollider = 0,
	AreaHit,
	Projectile
};

struct PendingCombatInteractionRecord
{
	Entity sourceEntity{ Entity::Null() };
	Entity sourceProxyEntity{ Entity::Null() };
	CombatHitSourceKind sourceKind{ CombatHitSourceKind::SkeletalCollider };
	AbilityId sourceAbilityId{ InvalidAbilityId };
	uint32_t sourceAbilityInstanceId{ 0 };
	uint16_t sourceAttackWindowIndex{ 0 };
	uint16_t sourceColliderIndex{ InvalidCombatColliderIndex };
	uint16_t targetColliderIndex{ InvalidCombatColliderIndex };
	CombatResolveResultType resultType{ CombatResolveResultType::Hit };
	AbilityAttackHitDef attackEffect{};
	std::optional<AbilityGuardResponseDef> guardEffect;
	std::optional<AbilityParryResponseDef> parryEffect;
	float maxKnockbackDistance{ 0.0f };
	float maxHitStopSec{ 0.0f };
	XMFLOAT3 impactPoint{ 0.0f, 0.0f, 0.0f };
	XMFLOAT3 swingDirection{ 0.0f, 0.0f, -1.0f };
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
	std::optional<GameplayEffectId> pendingParryEffectId;
};

struct PendingCombatImpactEvent
{
	Entity sourceEntity{ Entity::Null() };
	Entity sourceProxyEntity{ Entity::Null() };
	Entity targetEntity{ Entity::Null() };
	CombatHitSourceKind sourceKind{ CombatHitSourceKind::SkeletalCollider };
	AbilityId sourceAbilityId{ InvalidAbilityId };
	uint32_t sourceAbilityInstanceId{ 0 };
	uint16_t sourceAttackWindowIndex{ 0 };
	uint16_t sourceColliderIndex{ InvalidCombatColliderIndex };
	uint16_t targetColliderIndex{ InvalidCombatColliderIndex };
	CombatResolveResultType resultType{ CombatResolveResultType::Hit };
	XMFLOAT3 impactPoint{ 0.0f, 0.0f, 0.0f };
	XMFLOAT3 swingDirection{ 0.0f, 0.0f, -1.0f };
};

struct PendingCombatImpactEventComp : Component
{
	std::vector<PendingCombatImpactEvent> events;
};

struct PendingAreaHitRequest
{
	Entity sourceEntity{ Entity::Null() };
	Entity sourceProxyEntity{ Entity::Null() };
	AbilityId sourceAbilityId{ InvalidAbilityId };
	uint32_t sourceAbilityInstanceId{ 0 };
	uint16_t sourceEventIndex{ 0 };
	AreaHitId areaHitId{ InvalidAreaHitId };
	std::optional<std::string> areaHitKey;
	XMFLOAT3 origin{ 0.0f, 0.0f, 0.0f };
	XMFLOAT3 direction{ 0.0f, 0.0f, -1.0f };
	float elapsedSec{ 0.0f };
	bool spawnVolume{ false };
};

struct PendingAreaHitComp : Component
{
	std::vector<PendingAreaHitRequest> requests;
};

struct AreaVolumeStateComp : Component
{
	Entity owner{ Entity::Null() };
	AbilityId sourceAbilityId{ InvalidAbilityId };
	uint32_t sourceAbilityInstanceId{ 0 };
	AreaHitId areaHitId{ InvalidAreaHitId };
	std::optional<std::string> areaHitKey;
	XMFLOAT3 origin{ 0.0f, 0.0f, 0.0f };
	XMFLOAT3 direction{ 0.0f, 0.0f, -1.0f };
	float elapsedSec{ 0.0f };
	float lifetimeSec{ 0.0f };
	float tickIntervalSec{ 0.25f };
	float nextTickSec{ 0.0f };
};

struct AreaHitDedupStateComp : Component
{
	uint32_t boundSourceInstanceId{ 0 };
	std::vector<Entity> resolvedVictims;
};

struct ProjectileStateComp : Component
{
	ProjectileId projectileId{ InvalidProjectileId };
	std::optional<std::string> projectileKey;
	Entity owner{ Entity::Null() };
	AbilityId sourceAbilityId{ InvalidAbilityId };
	uint32_t sourceAbilityInstanceId{ 0 };
	XMFLOAT3 previousPosition{ 0.0f, 0.0f, 0.0f };
	XMFLOAT3 direction{ 0.0f, 0.0f, -1.0f };
	float speed{ 0.0f };
	float elapsedSec{ 0.0f };
	float travelledDistance{ 0.0f };
	int remainingPierceCount{ 0 };
};

struct ProjectileHitDedupStateComp : Component
{
	std::vector<Entity> resolvedVictims;
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

enum class ConsumableItemId : uint16_t
{
	None = 0,
	HpPotion = 1
};

struct HpPotionTuning
{
	uint16_t defaultGrantCount{ 3 };
	int32_t healAmount{ 50 };
};

struct ConsumableInventoryComp : Component
{
	uint16_t hpPotionCount{ 0 };
};

struct StaminaRecoveryTuning
{
	float baseRegenPerSec{ 45.0f };
	float spendRegenDelaySec{ 0.55f };
	float damageRegenDelaySec{ 0.75f };
	float exhaustedRegenDelaySec{ 1.0f };
	float guardRegenMultiplier{ 0.20f };
};

struct StaminaRecoveryStateComp : Component
{
	StaminaRecoveryTuning tuning;
	float regenLockRemainingSec{ 0.0f };
	float regenRemainder{ 0.0f };
};
