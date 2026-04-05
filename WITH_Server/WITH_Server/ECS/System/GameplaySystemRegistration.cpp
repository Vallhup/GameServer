#include "pch.h"
#include "GameplaySystemRegistration.h"

#include "../GameplayRuntimeComponents.h"
#include "Phase1/ApplyPlayerCommandSystem.h"
#include "Phase2/ResolveActionStateSystem.h"
#include "Phase2/ResolveLocomotionStateSystem.h"
#include "Phase3/FitSkeletalCombatColliderSystem.h"
#include "Phase3/ResolveAnimationPlaybackSystem.h"
#include "Phase3/SampleAnimationPoseSystem.h"
#include "Phase4/ApplyMovementDeltaSystem.h"
#include "Phase4/ComputeActionMoveDeltaSystem.h"
#include "Phase4/ComputeLocomotionMoveDeltaSystem.h"
#include "Phase5/ResolveCharacterOverlapSystem.h"
#include "Phase5/ResolveNavMeshBodyConstraintSystem.h"
#include "Phase6/MarkTransferPendingSystem.h"
#include "Phase6/ResolvePortalTriggerSystem.h"
#include "Phase7/ResolveCombatColliderActivationSystem.h"
#include "Phase7/ResolveCombatHitSystem.h"
#include "Phase8/CommitActionTimelineEventSystem.h"
#include "Phase8/CommitCombatResultSystem.h"
#include "Phase8/FinalizePostCommitStateSystem.h"
#include "Phase8/ResolveDeathAndDespawnSystem.h"
#include "Phase9/CollectReplicationTodoSourceSystem.h"
#include "WorldRuntime.h"

void RegisterGameplayRuntimeStorages(WorldRuntime& runtime)
{
	runtime.RegisterStorage<PlayerControlIdentityComp>();
	runtime.RegisterStorage<PlayerInputComp>();
	runtime.RegisterStorage<PendingDespawnTag>();
	runtime.RegisterStorage<PendingWorldTransferTag>();
	runtime.RegisterStorage<PendingWorldTransferComp>();
	runtime.RegisterStorage<PendingHitReactionComp>();
	runtime.RegisterStorage<PendingGuardBreakComp>();
	runtime.RegisterStorage<PendingKnockdownComp>();
	runtime.RegisterStorage<PendingBuffApplyComp>();
	runtime.RegisterStorage<PendingBuffRemoveComp>();
	runtime.RegisterStorage<ActionStateComp>();
	runtime.RegisterStorage<LocomotionStateComp>();
	runtime.RegisterStorage<ActionTimelineAdvanceComp>();
	runtime.RegisterStorage<AnimationPlaybackStateComp>();
	runtime.RegisterStorage<SampledAnimationPoseComp>();
	runtime.RegisterStorage<SkeletalCombatColliderComp>();
	runtime.RegisterStorage<WorldTransformComp>();
	runtime.RegisterStorage<LocomotionMoveDeltaComp>();
	runtime.RegisterStorage<ActionMoveDeltaComp>();
	runtime.RegisterStorage<ActionMoveRuntimeComp>();
	runtime.RegisterStorage<PreCollisionTransformComp>();
	runtime.RegisterStorage<BodyCollisionShapeComp>();
	runtime.RegisterStorage<NavMeshAgentStateComp>();
	runtime.RegisterStorage<BodyCollisionResolveComp>();
	runtime.RegisterStorage<PortalTriggerStateComp>();
	runtime.RegisterStorage<CombatColliderActivationComp>();
	runtime.RegisterStorage<CombatHitDedupStateComp>();
	runtime.RegisterStorage<PendingCombatResultComp>();
	runtime.RegisterStorage<CombatStatStateComp>();
	runtime.RegisterStorage<BuffRuntimeStateComp>();
	runtime.RegisterStorage<PendingProjectileSpawnComp>();
	runtime.RegisterStorage<PendingActionPresentationEventComp>();
	runtime.RegisterStorage<DirtyFlagsComp>();
	runtime.RegisterStorage<ReplicationStatsComp>();
}

void RegisterGameplayRuntimeSystems(
	WorldRuntime& runtime,
	const AnimationRegistry* animationRegistry)
{
	// Phase 1
	runtime.RegisterSystem<ApplyPlayerCommandSystem>(SystemPhase::Graph);

	// Phase 2
	runtime.RegisterSystem<ResolveActionStateSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<ResolveLocomotionStateSystem>(SystemPhase::Graph);

	// Phase 3
	runtime.RegisterSystem<ResolveAnimationPlaybackSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<SampleAnimationPoseSystem>(SystemPhase::Graph, animationRegistry);
	runtime.RegisterSystem<FitSkeletalCombatColliderSystem>(SystemPhase::Graph);

	// Phase 4
	runtime.RegisterSystem<ComputeLocomotionMoveDeltaSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<ComputeActionMoveDeltaSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<ApplyMovementDeltaSystem>(SystemPhase::Graph);

	// Phase 5
	runtime.RegisterSystem<ResolveNavMeshBodyConstraintSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<ResolveCharacterOverlapSystem>(SystemPhase::Graph);

	// Phase 6
	runtime.RegisterSystem<ResolvePortalTriggerSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<MarkTransferPendingSystem>(SystemPhase::Graph);

	// Phase 7
	runtime.RegisterSystem<ResolveCombatColliderActivationSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<ResolveCombatHitSystem>(SystemPhase::Graph);

	// Phase 8
	runtime.RegisterSystem<CommitCombatResultSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<CommitActionTimelineEventSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<ResolveDeathAndDespawnSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<FinalizePostCommitStateSystem>(SystemPhase::Graph);

	// Phase 9
	runtime.RegisterSystem<CollectReplicationTodoSourceSystem>(SystemPhase::Graph);
}
