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

#include "AISensingSystem.h"
#include "AITacticSystem.h"
#include "AIActionRequestSystem.h"
#include "AICommandBuildSystem.h"
#include "AICommandConsumeSystem.h"

#include "LifecycleReplicationSystem.h"
#include "DirtyReplicationSystem.h"

#include "Tags.h"

TestWorldImpl::TestWorldImpl()
{
}

void TestWorldImpl::SpawnInitial(WorldRuntime& rt)
{
	ECS& ecs = rt.GetECS();
	Entity e = ecs.CreateEntity();

	ecs.GetStorage<Transform>().AddComponent(e)->position = 
	{ 10.0f, MapCollisionManager::Get().SampleHeightAt(10.f, 10.f), 10.0f };
	ecs.GetStorage<Velocity>().AddComponent(e);
	ecs.GetStorage<ActionMoveDelta>().AddComponent(e);
	ecs.GetStorage<LocomotionMoveDelta>().AddComponent(e);
	ecs.GetStorage<LocomotionAnimPhase>().AddComponent(e);
	ecs.GetStorage<LocomotionState>().AddComponent(e);
	ecs.GetStorage<ActionState>().AddComponent(e);
	ecs.GetStorage<BaseAttribute>().AddComponent(e)->moveSpeed = 2.5;
	ecs.GetStorage<FinalAttribute>().AddComponent(e);
	ecs.GetStorage<BaseVital>().AddComponent(e);
	ecs.GetStorage<FinalVital>().AddComponent(e);
	ecs.GetStorage<Vital>().AddComponent(e);
	ecs.GetStorage<AnimationState>().AddComponent(e);
	auto animator = ecs.GetStorage<Animator>().AddComponent(e);
	ecs.GetStorage<CombatCollider>().AddComponent(e);
	ecs.GetStorage<AttackState>().AddComponent(e);
	ecs.GetStorage<SpawnTypeComp>().AddComponent(e)->type = EntityType::Final_Boss;
	ecs.GetStorage<AIState>().AddComponent(e);
	ecs.GetStorage<AIThinkState>().AddComponent(e);
	ecs.GetStorage<AISenseState>().AddComponent(e);
	ecs.GetStorage<AIBehavior>().AddComponent(e);
	ecs.GetStorage<AICommand>().AddComponent(e);
	ecs.GetStorage<AICombatTuning>().AddComponent(e);
	ecs.GetStorage<AIActionRequestState>().AddComponent(e);
	ecs.GetStorage<DirtyFlagsComp>().AddComponent(e);

	Framework& framework = Framework::Get();
	NetId id = framework.netIdRegistry.Allocate();

	ecs.GetStorage<NetIdComp>().AddComponent(e)->id = id;
	framework.netIdRegistry.BindEntity(id, e);

	animator->clip = 
		AnimationManager::Get().GetAnimation(AnimationType::FinalBoss_Idle);
}

Entity TestWorldImpl::SpawnPlayer(WorldRuntime& rt, uint32 connId)
{
	ECS& ecs = rt.GetECS();
	Entity e = ecs.CreateEntity();

	ecs.GetStorage<Transform>().AddComponent(e)->position = 
	{ 10.0f, MapCollisionManager::Get().SampleHeightAt(10.f, 10.f), 10.0f };
	ecs.GetStorage<Velocity>().AddComponent(e);
	ecs.GetStorage<ActionMoveDelta>().AddComponent(e);
	ecs.GetStorage<LocomotionMoveDelta>().AddComponent(e);
	ecs.GetStorage<LocomotionAnimPhase>().AddComponent(e);
	ecs.GetStorage<LocomotionState>().AddComponent(e);
	ecs.GetStorage<ActionIntent>().AddComponent(e);
	ecs.GetStorage<ActionState>().AddComponent(e);
	ecs.GetStorage<BaseAttribute>().AddComponent(e);
	ecs.GetStorage<FinalAttribute>().AddComponent(e);
	ecs.GetStorage<BaseVital>().AddComponent(e);
	ecs.GetStorage<FinalVital>().AddComponent(e);
	ecs.GetStorage<Vital>().AddComponent(e);
	ecs.GetStorage<BuffsComp>().AddComponent(e);
	ecs.GetStorage<AnimationState>().AddComponent(e);
	auto animator = ecs.GetStorage<Animator>().AddComponent(e);
	ecs.GetStorage<CombatCollider>().AddComponent(e);
	ecs.GetStorage<AttackState>().AddComponent(e);
	ecs.GetStorage<ParryBuf>().AddComponent(e);
	ecs.GetStorage<PlayerTag>().AddComponent(e);
	ecs.GetStorage<SpawnTypeComp>().AddComponent(e)->type = EntityType::Knight;
	
	ecs.GetStorage<WorldIdComp>().AddComponent(e);
	ecs.GetStorage<DirtyFlagsComp>().AddComponent(e);

	Framework& framework = Framework::Get();
	NetId nId = framework.netIdRegistry.Allocate();

	ecs.GetStorage<NetIdComp>().AddComponent(e)->id = nId;
	framework.listener.GetIdMap().BindPlayer(connId, nId);
	framework.netIdRegistry.BindEntity(nId, e);

	animator->clip = 
		AnimationManager::Get().GetAnimation(AnimationType::Knight_Idle);

	return e;
}

void TestWorldImpl::Build(WorldRuntime& rt)
{
	auto& ecs = rt.GetECS();

	ecs.AddSystem<EventSystem>(SystemPhase::Pre, rt, 0);
	ecs.AddSystem<AISensingSystem>(SystemPhase::Pre, rt, 0);
	ecs.AddSystem<AITacticSystem>(SystemPhase::Pre, rt, 0);
	ecs.AddSystem<AIActionRequestSystem>(SystemPhase::Pre, rt, 0);
	ecs.AddSystem<AICommandBuildSystem>(SystemPhase::Pre, rt, 0);
	ecs.AddSystem<AICommandConsumeSystem>(SystemPhase::Pre, rt, 0);

	ecs.AddSystem<ActionTimeSystem>(SystemPhase::Graph, rt, 1);
	ecs.AddSystem<ActionTransitionSystem>(SystemPhase::Graph, rt, 2);

	ecs.AddSystem<BuffApplySystem>(SystemPhase::Graph, rt, 3);
	ecs.AddSystem<StatRecalSystem>(SystemPhase::Graph, rt, 4);

	ecs.AddSystem<ActionMoveSystem>(SystemPhase::Graph, rt, 11);
	ecs.AddSystem<LocomotionMoveSystem>(SystemPhase::Graph, rt, 12);
	ecs.AddSystem<MovementApplySystem>(SystemPhase::Graph, rt, 13);

	// TODO : 공간분할

	ecs.AddSystem<AnimationSelectSystem>(SystemPhase::Graph, rt, 21);
	ecs.AddSystem<AnimationCommitSystem>(SystemPhase::Graph, rt, 22);
	ecs.AddSystem<AnimationFrameSystem>(SystemPhase::Graph, rt, 23);
	ecs.AddSystem<AnimationPoseBindSystem>(SystemPhase::Graph, rt, 24);

	ecs.AddSystem<ColliderUpdateSystem>(SystemPhase::Graph, rt, 31);
	ecs.AddSystem<ColliderActivationSystem>(SystemPhase::Graph, rt, 32);
	ecs.AddSystem<CombatCollisionCheckSystem>(SystemPhase::Graph, rt, 33);
	ecs.AddSystem<CombatCollisionDedupSystem>(SystemPhase::Graph, rt, 34);
	ecs.AddSystem<CombatCollisionHandlingSystem>(SystemPhase::Graph, rt, 35);

	// TODO : 시야처리

	ecs.AddSystem<DirtyReplicationSystem>(SystemPhase::Post, rt, 100);
	ecs.AddSystem<LifecycleReplicationSystem>(SystemPhase::Post, rt, 101);
}

void TestWorldImpl::OnShutdown(WorldRuntime& rt)
{

}