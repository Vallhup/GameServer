#include "pch.h"
#include "GameplaySystemRegistration.h"

#include "../GameplayRuntimeComponents.h"
#include "Phase0_AI/AIDecisionSystem.h"
#include "Phase0_AI/AIPerceptionSystem.h"
#include "Phase1/ApplyAICommandSystem.h"
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
	runtime.RegisterStorage<ActorInputComp>();
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

	// AI 전용 컴포넌트
	runtime.RegisterStorage<AIControlledTag>();
	runtime.RegisterStorage<AIPerceptionComp>();
	runtime.RegisterStorage<AIPerceptionTuningComp>();
	runtime.RegisterStorage<AIBlackboardComp>();
	runtime.RegisterStorage<AIDecisionComp>();
	runtime.RegisterStorage<AIDecisionTuningComp>();
	runtime.RegisterStorage<AIReactionComp>();
	runtime.RegisterStorage<AICommandFrameComp>();
}

void RegisterGameplayRuntimeSystems(
	WorldRuntime& runtime,
	const AnimationRegistry* animationRegistry)
{
	// Pre-Phase 1: AI 시스템
	runtime.RegisterSystem<AIPerceptionSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<AIDecisionSystem>(SystemPhase::Graph);

	// Phase 1: 명령 적용
	runtime.RegisterSystem<ApplyPlayerCommandSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<ApplyAICommandSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<ResolveActionStateSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<ResolveLocomotionStateSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<ResolveAnimationPlaybackSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<SampleAnimationPoseSystem>(
		SystemPhase::Graph,
		animationRegistry);
	runtime.RegisterSystem<FitSkeletalCombatColliderSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<ComputeLocomotionMoveDeltaSystem>(
		SystemPhase::Graph);
	runtime.RegisterSystem<ComputeActionMoveDeltaSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<ApplyMovementDeltaSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<ResolveNavMeshBodyConstraintSystem>(
		SystemPhase::Graph);
	runtime.RegisterSystem<ResolveCharacterOverlapSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<ResolvePortalTriggerSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<MarkTransferPendingSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<ResolveCombatColliderActivationSystem>(
		SystemPhase::Graph);
	runtime.RegisterSystem<ResolveCombatHitSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<CommitCombatResultSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<CommitActionTimelineEventSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<ResolveDeathAndDespawnSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<FinalizePostCommitStateSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<CollectReplicationTodoSourceSystem>(
		SystemPhase::Graph);
}
