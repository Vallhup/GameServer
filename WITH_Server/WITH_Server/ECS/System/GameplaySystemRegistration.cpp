#include "pch.h"
#include "GameplaySystemRegistration.h"

#include "Phase0_AI/AIDecisionSystem.h"
#include "Phase0_AI/AIPerceptionSystem.h"
#include "Phase1/ApplyAICommandSystem.h"
#include "Phase1/ApplyPlayerCommandSystem.h"
#include "Phase2/AdvanceActionTimelineSystem.h"
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
	runtime.RegisterSystem<AdvanceActionTimelineSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<ResolveLocomotionStateSystem>(SystemPhase::Graph);

	// Phase 3
	runtime.RegisterSystem<ResolveAnimationPlaybackSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<SampleAnimationPoseSystem>(SystemPhase::Graph, animationRegistry);
	runtime.RegisterSystem<FitSkeletalCombatColliderSystem>(
		SystemPhase::Graph,
		animationRegistry);

	// Phase 4
	runtime.RegisterSystem<ComputeLocomotionMoveDeltaSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<ComputeActionMoveDeltaSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<ApplyMovementDeltaSystem>(SystemPhase::Graph);

	// Phase 5
	runtime.RegisterSystem<ResolveCharacterOverlapSystem>(SystemPhase::Graph);
	runtime.RegisterSystem<ResolveNavMeshBodyConstraintSystem>(SystemPhase::Graph);

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
