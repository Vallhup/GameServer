#include "pch.h"
#include "TestWorldImpl.h"
#include "EventSystem.h"
#include "OutputEventSystem.h"
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

TestWorldImpl::TestWorldImpl()
{
}

void TestWorldImpl::SpawnInitial(WorldRuntime& rt)
{
	ECS& ecs = rt.GetECS();
	Entity e = ecs.CreateEntity();

	ecs.GetStorage<Transform>().AddComponent(e);
	ecs.GetStorage<Velocity>().AddComponent(e);
	ecs.GetStorage<ActionMoveDelta>().AddComponent(e);
	ecs.GetStorage<LocomotionMoveDelta>().AddComponent(e);
	ecs.GetStorage<LocomotionAnimPhase>().AddComponent(e);
	ecs.GetStorage<LocomotionState>().AddComponent(e);
	ecs.GetStorage<ActionState>().AddComponent(e);
	ecs.GetStorage<AttackData>().AddComponent(e);
	ecs.GetStorage<Health>().AddComponent(e);
	ecs.GetStorage<AnimationState>().AddComponent(e);
	auto animator = ecs.GetStorage<Animator>().AddComponent(e);
	ecs.GetStorage<CombatCollider>().AddComponent(e);
	ecs.GetStorage<AttackState>().AddComponent(e);

	animator->clip = AnimationManager::Get().GetAnimation()
}

Entity TestWorldImpl::SpawnPlayer(WorldRuntime& rt, uint32 connId)
{
	ECS& ecs = rt.GetECS();
	Entity e = ecs.CreateEntity();

	ecs.GetStorage<Transform>().AddComponent(e);
	ecs.GetStorage<Velocity>().AddComponent(e);
	ecs.GetStorage<ActionMoveDelta>().AddComponent(e);
	ecs.GetStorage<LocomotionMoveDelta>().AddComponent(e);
	ecs.GetStorage<LocomotionAnimPhase>().AddComponent(e);
	ecs.GetStorage<LocomotionState>().AddComponent(e);
	ecs.GetStorage<ActionIntent>().AddComponent(e);
	ecs.GetStorage<ActionState>().AddComponent(e);
	ecs.GetStorage<AttackData>().AddComponent(e);
	ecs.GetStorage<Health>().AddComponent(e);
	ecs.GetStorage<AnimationState>().AddComponent(e);
	auto animator = ecs.GetStorage<Animator>().AddComponent(e);
	ecs.GetStorage<CombatCollider>().AddComponent(e);
	ecs.GetStorage<AttackState>().AddComponent(e);
	ecs.GetStorage<ParryBuf>().AddComponent(e);
	ecs.GetStorage<PlayerTag>().AddComponent(e);

	animator->clip = 
		AnimationManager::Get().GetAnimation(AnimationType::FinalBoss_Idle);

	return e;
}

void TestWorldImpl::Build(WorldRuntime& rt)
{
	auto& ecs = rt.GetECS();

	ecs.AddSystem<EventSystem>(SystemPhase::Pre, ecs, 0);

	ecs.AddSystem<ActionTimeSystem>(SystemPhase::Graph, ecs, 1);
	ecs.AddSystem<ActionTransitionSystem>(SystemPhase::Graph, ecs, 2);

	ecs.AddSystem<ActionMoveSystem>(SystemPhase::Graph, ecs, 11);
	ecs.AddSystem<LocomotionMoveSystem>(SystemPhase::Graph, ecs, 12);
	ecs.AddSystem<MovementApplySystem>(SystemPhase::Graph, ecs, 13);

	// TODO : 공간분할

	ecs.AddSystem<AnimationSelectSystem>(SystemPhase::Graph, ecs, 21);
	ecs.AddSystem<AnimationCommitSystem>(SystemPhase::Graph, ecs, 22);
	ecs.AddSystem<AnimationFrameSystem>(SystemPhase::Graph, ecs, 23);
	ecs.AddSystem<AnimationPoseBindSystem>(SystemPhase::Graph, ecs, 24);

	ecs.AddSystem<ColliderUpdateSystem>(SystemPhase::Graph, ecs, 31);
	ecs.AddSystem<ColliderActivationSystem>(SystemPhase::Graph, ecs, 32);
	ecs.AddSystem<CombatCollisionCheckSystem>(SystemPhase::Graph, ecs, 33);
	ecs.AddSystem<CombatCollisionDedupSystem>(SystemPhase::Graph, ecs, 34);
	ecs.AddSystem<CombatCollisionHandlingSystem>(SystemPhase::Graph, ecs, 35);

	// TODO : 시야처리

	ecs.AddSystem<OutputEventSystem>(SystemPhase::Post, ecs, 100);
}

void TestWorldImpl::Execute(WorldRuntime& rt, double dT)
{
	rt.Run(dT);
}

void TestWorldImpl::OnShutdown(WorldRuntime& rt)
{

}