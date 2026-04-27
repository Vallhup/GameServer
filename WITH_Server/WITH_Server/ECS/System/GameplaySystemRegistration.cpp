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
	target.RegisterSystem<AIPerceptionSystem>(SystemPhase::Graph);
	target.RegisterSystem<AIDecisionSystem>(SystemPhase::Graph);

	target.RegisterSystem<ApplyPlayerCommandSystem>(SystemPhase::Graph);
	target.RegisterSystem<ApplyAICommandSystem>(SystemPhase::Graph);
	target.RegisterSystem<ResolveActionStateSystem>(SystemPhase::Graph);
	target.RegisterSystem<AdvanceActionTimelineSystem>(SystemPhase::Graph);
	target.RegisterSystem<ResolveLocomotionStateSystem>(SystemPhase::Graph);

	target.RegisterSystem<ResolveAnimationPlaybackSystem>(SystemPhase::Graph);
	target.RegisterSystem<SampleAnimationPoseSystem>(
		SystemPhase::Graph,
		_animationRegistry);
	target.RegisterSystem<FitSkeletalCombatColliderSystem>(
		SystemPhase::Graph,
		_animationRegistry);

	target.RegisterSystem<ComputeLocomotionMoveDeltaSystem>(SystemPhase::Graph);
	target.RegisterSystem<ComputeActionMoveDeltaSystem>(SystemPhase::Graph);
	target.RegisterSystem<ApplyMovementDeltaSystem>(SystemPhase::Graph);

	target.RegisterSystem<ResolveCharacterOverlapSystem>(SystemPhase::Graph);
	target.RegisterSystem<ResolveNavMeshBodyConstraintSystem>(SystemPhase::Graph);

	target.RegisterSystem<ResolvePortalTriggerSystem>(SystemPhase::Graph);
	target.RegisterSystem<MarkTransferPendingSystem>(SystemPhase::Graph);

	target.RegisterSystem<ResolveCombatColliderActivationSystem>(SystemPhase::Graph);
	target.RegisterSystem<ResolveCombatHitSystem>(SystemPhase::Graph);

	target.RegisterSystem<CommitCombatResultSystem>(SystemPhase::Graph);
	target.RegisterSystem<CommitActionTimelineEventSystem>(SystemPhase::Graph);
	target.RegisterSystem<ResolveDeathAndDespawnSystem>(SystemPhase::Graph);
	target.RegisterSystem<FinalizePostCommitStateSystem>(SystemPhase::Graph);

	target.RegisterSystem<CollectReplicationTodoSourceSystem>(SystemPhase::Graph);
}

template void GameplaySystemRegistrar::RegisterSystems<WorldRuntime>(
	WorldRuntime& target) const;
template void GameplaySystemRegistrar::RegisterSystems<SystemManager>(
	SystemManager& target) const;
