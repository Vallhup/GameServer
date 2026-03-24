#include "pch.h"
#include "TestWorldImpl.h"
#include "EventSystem.h"
#include "ColliderUpdateSystem.h"
#include "ActionTimeSystem.h"
#include "ActionTransitionSystem.h"
#include "ActionMoveSystem.h"
#include "LocomotionMoveSystem.h"
#include "MovementApplySystem.h"
#include "AnimationCommitSystem.h"
#include "AnimationFrameSystem.h"
#include "AnimationPoseBindSystem.h"
#include "AnimationSelectSystem.h"
#include "ColliderActivationSystem.h"
#include "CombatCollisionCheckSystem.h"
#include "CombatCollisionDedupSystem.h"
#include "CombatCollisionHandlingSystem.h"
#include "BuffApplySystem.h"
#include "StatRecalSystem.h"

#include "RepComponent.h"
#include "Framework.h"

#include "AIPerceptionSystem.h"
#include "AIDecisionSystem.h"
#include "AIExecutionSystem.h"

#include "LifecycleReplicationSystem.h"
#include "DirtyReplicationSystem.h"

#include "Tags.h"

TestWorldImpl::TestWorldImpl()
{
}

void TestWorldImpl::Configure(WorldBuilder& builder)
{
	builder.Components<Transform, Velocity, ActionMoveDelta,
		LocomotionMoveDelta, LocomotionAnimPhase, LocomotionState,
		ActionState, ActionIntent, BaseAttribute, FinalAttribute,
		BaseVital, FinalVital, Vital, AnimationState, Animator,
		CombatCollider, AttackState, SpawnTypeComp, AIPerceptionCache, 
		AIPerceptionTuning, AIBlackboard, AIDecisionState, AIDecisionTuning, 
		AIReactionCache, AIIntent, AIExecutionState, EntityCommandFrame, DirtyFlagsComp,
		NetIdComp, WorldIdComp, ParryBuf, PlayerTag, DisconnectedTag,
		ActionMoveTag, BuffsComp>()
		.PreSystem<EventSystem>()
		.PreSystem<AIPerceptionSystem>()
		.PreSystem<AIDecisionSystem>()
		.PreSystem<AIExecutionSystem>()
		.GraphSystem<ActionTimeSystem>()
		.GraphSystem<ActionTransitionSystem>()
		.GraphSystem<BuffApplySystem>()
		.GraphSystem<StatRecalSystem>()
		.GraphSystem<ActionMoveSystem>()
		.GraphSystem<LocomotionMoveSystem>()
		.GraphSystem<MovementApplySystem>()
		.GraphSystem<AnimationSelectSystem>()
		.GraphSystem<AnimationCommitSystem>()
		.GraphSystem<AnimationFrameSystem>()
		.GraphSystem<AnimationPoseBindSystem>()
		.GraphSystem<ColliderUpdateSystem>()
		.GraphSystem<ColliderActivationSystem>()
		.GraphSystem<CombatCollisionCheckSystem>()
		.GraphSystem<CombatCollisionDedupSystem>()
		.GraphSystem<CombatCollisionHandlingSystem>()
		.PostSystem<DirtyReplicationSystem>()
		.PostSystem<LifecycleReplicationSystem>()
		.BeginInitialSpawns();

	Framework& framework = Framework::Get();
	NetId netId = framework.netIdRegistry.Allocate();

	Entity entity = builder.Spawn(
		Transform{ .position = { 10.0f, MapCollisionManager::Get().SampleHeightAt(10.f, 10.f), 10.0f } },
		Velocity{}, ActionMoveDelta{}, LocomotionMoveDelta{},
		LocomotionAnimPhase{}, LocomotionState{}, ActionState{},
		BaseAttribute{ .moveSpeed = 2.5 }, FinalAttribute{},
		BaseVital{}, FinalVital{}, Vital{}, AnimationState{},
		Animator{ .clip = AnimationManager::Get().GetAnimation(AnimationType::Imp_Idle) },
		CombatCollider{}, AttackState{}, SpawnTypeComp{ .entityType = EntityType::Character, .faction = Faction::Enemy, .charType = CharacterType::Imp },
		AIPerceptionCache{}, AIPerceptionTuning{}, AIBlackboard{}, AIDecisionState{}, AIDecisionTuning{},
		AIReactionCache{}, AIIntent{}, AIExecutionState{}, EntityCommandFrame{}, DirtyFlagsComp{}, NetIdComp{ .id = netId }
	);

	framework.netIdRegistry.BindEntity(netId, entity);
}

Entity TestWorldImpl::SpawnPlayer(WorldRuntime& rt, uint32 connId)
{
	ECS& ecs = rt.GetECS();
	Entity e = ecs.CreateEntityImmediate();

	Transform tr{ .position = { 10.0f, MapCollisionManager::Get().SampleHeightAt(10.f, 10.f), 10.0f } };

	rt.ImmediateAddComponent<Transform>(e, std::move(tr));

	rt.ImmediateAddComponent<Velocity>(e);
	rt.ImmediateAddComponent<ActionMoveDelta>(e);
	rt.ImmediateAddComponent<LocomotionMoveDelta>(e);
	rt.ImmediateAddComponent<LocomotionAnimPhase>(e);
	rt.ImmediateAddComponent<LocomotionState>(e);
	rt.ImmediateAddComponent<ActionIntent>(e);
	rt.ImmediateAddComponent<ActionState>(e);
	rt.ImmediateAddComponent<BaseAttribute>(e);
	rt.ImmediateAddComponent<FinalAttribute>(e);
	rt.ImmediateAddComponent<BaseVital>(e);
	rt.ImmediateAddComponent<FinalVital>(e);
	rt.ImmediateAddComponent<Vital>(e);
	rt.ImmediateAddComponent<BuffsComp>(e);
	rt.ImmediateAddComponent<AnimationState>(e);

	Animator animator{ .clip = AnimationManager::Get().GetAnimation(AnimationType::Knight_Idle) };

	rt.ImmediateAddComponent<Animator>(e, std::move(animator));

	rt.ImmediateAddComponent<CombatCollider>(e);
	rt.ImmediateAddComponent<AttackState>(e);
	rt.ImmediateAddComponent<ParryBuf>(e);
	rt.ImmediateAddComponent<PlayerTag>(e);

	SpawnTypeComp typeComp;
	typeComp.entityType = EntityType::Character;
	typeComp.faction = Faction::Player;
	typeComp.charType = CharacterType::Knight;

	rt.ImmediateAddComponent<SpawnTypeComp>(e, std::move(typeComp));

	rt.ImmediateAddComponent<WorldIdComp>(e);
	rt.ImmediateAddComponent<DirtyFlagsComp>(e);

	Framework& framework = Framework::Get();
	NetId nId = framework.netIdRegistry.Allocate();

	NetIdComp idComp{ .id = nId };

	rt.ImmediateAddComponent<NetIdComp>(e, std::move(idComp));

	/*rt.DeferredUpsertComponent<Transform>(e,
		[&](Transform& tr)
		{
			tr.position = 
			{ 10.0f, MapCollisionManager::Get().SampleHeightAt(10.f, 10.f), 10.0f };
		});

	rt.DeferredAddComponent<Velocity>(e);
	rt.DeferredAddComponent<ActionMoveDelta>(e);
	rt.DeferredAddComponent<LocomotionMoveDelta>(e);
	rt.DeferredAddComponent<LocomotionAnimPhase>(e);
	rt.DeferredAddComponent<LocomotionState>(e);
	rt.DeferredAddComponent<ActionIntent>(e);
	rt.DeferredAddComponent<ActionState>(e);
	rt.DeferredAddComponent<BaseAttribute>(e);
	rt.DeferredAddComponent<FinalAttribute>(e);
	rt.DeferredAddComponent<BaseVital>(e);
	rt.DeferredAddComponent<FinalVital>(e);
	rt.DeferredAddComponent<Vital>(e);
	rt.DeferredAddComponent<BuffsComp>(e);
	rt.DeferredAddComponent<AnimationState>(e);

	rt.DeferredUpsertComponent<Animator>(e,
		[&](Animator& animator)
		{
			animator.clip =
				AnimationManager::Get().GetAnimation(AnimationType::Knight_Idle);
		});

	rt.DeferredAddComponent<CombatCollider>(e);
	rt.DeferredAddComponent<AttackState>(e);
	rt.DeferredAddComponent<ParryBuf>(e);
	rt.DeferredAddComponent<PlayerTag>(e);

	rt.DeferredUpsertComponent<SpawnTypeComp>(e,
		[&](SpawnTypeComp& typeComp)
		{
			typeComp.type = EntityType::Knight;
		});

	rt.DeferredAddComponent<WorldIdComp>(e);
	rt.DeferredAddComponent<DirtyFlagsComp>(e);

	Framework& framework = Framework::Get();
	NetId nId = framework.netIdRegistry.Allocate();

	rt.DeferredUpsertComponent<NetIdComp>(e,
		[nId](NetIdComp& idComp)
		{
			idComp.id = nId;
		});*/

	framework.listener.GetIdMap().BindPlayer(connId, nId);
	framework.netIdRegistry.BindEntity(nId, e);

	return e;
}

void TestWorldImpl::OnShutdown(WorldRuntime& rt)
{

}