#include "pch.h"
#include "GameplaySystemRegistration.h"

#include "Phase0_AI/AIDecisionSystem.h"
#include "Phase0_AI/AIPerceptionSystem.h"
#include "Phase1/ApplyAICommandSystem.h"
#include "Phase1/ApplyPlayerCommandSystem.h"
#include "Phase2/AdvanceAbilityTimelineSystem.h"
#include "Phase2/ResolveAbilityStateSystem.h"
#include "Phase2/ResolveLocomotionStateSystem.h"
#include "Phase3/FitSkeletalCombatColliderSystem.h"
#include "Phase3/ResolveAnimationPlaybackSystem.h"
#include "Phase3/SampleAnimationPoseSystem.h"
#include "Phase4/ApplyMovementDeltaSystem.h"
#include "Phase4/ComputeAbilityMoveDeltaSystem.h"
#include "Phase4/ComputeLocomotionMoveDeltaSystem.h"
#include "Phase5/ResolveCharacterOverlapSystem.h"
#include "Phase5/ResolveNavMeshBodyConstraintSystem.h"
#include "Phase6/MarkTransferPendingSystem.h"
#include "Phase6/AdvanceProjectileSystem.h"
#include "Phase6/MaterializeAbilityGameplayEventSystem.h"
#include "Phase6/ResolvePortalTriggerSystem.h"
#include "Phase6/SpawnAreaVolumeSystem.h"
#include "Phase6/SpawnProjectileSystem.h"
#include "Phase7/ResolveCombatColliderActivationSystem.h"
#include "Phase7/ResolveAreaHitSystem.h"
#include "Phase7/ResolveCombatHitSystem.h"
#include "Phase7/ResolveProjectileHitSystem.h"
#include "Phase8/CommitAbilityTimelineEventSystem.h"
#include "Phase8/CommitCombatResultSystem.h"
#include "Phase8/FinalizePostCommitStateSystem.h"
#include "Phase8/RecoverStaminaSystem.h"
#include "Phase8/ResolveGameplayEffectStateSystem.h"
#include "Phase8/ResolveDeathAndDespawnSystem.h"
#include "Phase9/CollectReplicationTodoSourceSystem.h"
#include "SystemManager.h"
#include "WorldRuntime.h"

GameplaySystemRegistrar::GameplaySystemRegistrar(
	const AnimationRegistry* animationRegistry) noexcept
	: _animationRegistry(animationRegistry)
{
}

void GameplaySystemRegistrar::Register(WorldRuntime& runtime) const
{
	RegisterSystems(runtime);
}

void GameplaySystemRegistrar::Register(SystemManager& systemManager) const
{
	RegisterSystems(systemManager);
}

template<typename TargetT>
void GameplaySystemRegistrar::RegisterSystems(TargetT& target) const
{
	target.RegisterSystem<AIPerceptionSystem>();
	target.RegisterSystem<AIDecisionSystem>();

	target.RegisterSystem<ApplyPlayerCommandSystem>();
	target.RegisterSystem<ApplyAICommandSystem>();
	target.RegisterSystem<ResolveAbilityStateSystem>();
	target.RegisterSystem<AdvanceAbilityTimelineSystem>();
	target.RegisterSystem<ResolveLocomotionStateSystem>();

	target.RegisterSystem<ResolveAnimationPlaybackSystem>();
	target.RegisterSystem<SampleAnimationPoseSystem>(_animationRegistry);
	target.RegisterSystem<FitSkeletalCombatColliderSystem>(_animationRegistry);

	target.RegisterSystem<ComputeLocomotionMoveDeltaSystem>();
	target.RegisterSystem<ComputeAbilityMoveDeltaSystem>();
	target.RegisterSystem<ApplyMovementDeltaSystem>();

	target.RegisterSystem<ResolveCharacterOverlapSystem>();
	target.RegisterSystem<ResolveNavMeshBodyConstraintSystem>();

	target.RegisterSystem<ResolvePortalTriggerSystem>();
	target.RegisterSystem<MarkTransferPendingSystem>();
	target.RegisterSystem<MaterializeAbilityGameplayEventSystem>();
	target.RegisterSystem<SpawnProjectileSystem>();
	target.RegisterSystem<SpawnAreaVolumeSystem>();
	target.RegisterSystem<AdvanceProjectileSystem>();

	target.RegisterSystem<ResolveCombatColliderActivationSystem>();
	target.RegisterSystem<ResolveProjectileHitSystem>();
	target.RegisterSystem<ResolveAreaHitSystem>();
	target.RegisterSystem<ResolveCombatHitSystem>();

	target.RegisterSystem<CommitCombatResultSystem>();
	target.RegisterSystem<RecoverStaminaSystem>();
	target.RegisterSystem<CommitAbilityTimelineEventSystem>();
	target.RegisterSystem<ResolveGameplayEffectStateSystem>();
	target.RegisterSystem<ResolveDeathAndDespawnSystem>();
	target.RegisterSystem<FinalizePostCommitStateSystem>();

	target.RegisterSystem<CollectReplicationTodoSourceSystem>();
}

template void GameplaySystemRegistrar::RegisterSystems<WorldRuntime>(
	WorldRuntime& target) const;
template void GameplaySystemRegistrar::RegisterSystems<SystemManager>(
	SystemManager& target) const;
