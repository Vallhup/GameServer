#include "pch.h"
#include "DirtyReplicationSystem.h"
#include "Framework.h"
#include "Tags.h"
#include "Animation.h"
#include "Stats.h"

void DirtyReplicationSystem::Execute(const double dT)
{
	ECS& ecs = _runtime.GetECS();
	auto& dirtyEntities = _runtime.DirtyEntities();

	for (Entity entity : dirtyEntities)
	{
		auto* dirty = ecs.GetStorage<DirtyFlagsComp>().GetComponent(entity);
		if (!dirty || !dirty->AnyDirty()) continue;

		FlushEntity(entity, *dirty);
		dirty->Clear();
	}

	dirtyEntities.clear();
}

void DirtyReplicationSystem::FlushEntity(Entity entity, const DirtyFlagsComp& dirty)
{
	Framework& framework = Framework::Get();
	ECS& ecs = _runtime.GetECS();

	if (ecs.GetStorage<DisconnectedTag>().HasComponent(entity))
		return;

	const auto* netComp = ecs.GetStorage<NetIdComp>().GetComponent(entity);
	if (!netComp) return;

	const NetId netId = netComp->id;
	const uint32_t connId = framework.listener.GetIdMap().GetConn(netId);

	if (dirty.IsDirty(WorldDirtyType::Transform))
	{
		FlushTransform(entity, netId);
	}

	if (dirty.IsDirty(WorldDirtyType::Animation))
	{
		FlushAnimation(entity, netId);
	}

	if (dirty.IsDirty(WorldDirtyType::Stat))
	{
		FlushStat(entity, netId, connId);
	}
}

void DirtyReplicationSystem::FlushTransform(Entity entity, NetId netId)
{
	Framework& framework = Framework::Get();
	ECS& ecs = _runtime.GetECS();

	const auto* trans = ecs.GetStorage<Transform>().GetComponent(entity);
	if (!trans) return;

	SendBuffer* data = NetHelper::SCMovePacket(
		netId,
		trans->position.x, trans->position.y, trans->position.z,
		TransformHelper::QuaternionToYaw(trans->rotation)
	);

	std::vector<uint32_t> connIds;
	framework.listener.GetConnRegistry().FillConnIds(connIds);

	for (uint32_t connId : connIds)
	{
		framework.listener.SendBuffers().
			Enqueue(connId, data->data, data->size);
	}

	SendBufferPool::Get().Release(data);
}

void DirtyReplicationSystem::FlushAnimation(Entity entity, NetId netId)
{
	Framework& framework = Framework::Get();
	ECS& ecs = _runtime.GetECS();

	const auto* animState = ecs.GetStorage<AnimationState>().GetComponent(entity);
	if (!animState) return;

	SendBuffer* data = NetHelper::SCAnimationChangePacket(
		netId, animState->desiredId
	);

	std::vector<uint32_t> connIds;
	framework.listener.GetConnRegistry().FillConnIds(connIds);

	for (uint32_t connId : connIds)
	{
		framework.listener.SendBuffers().
			Enqueue(connId, data->data, data->size);
	}

	SendBufferPool::Get().Release(data);
}

void DirtyReplicationSystem::FlushStat(Entity entity, NetId netId, uint32_t connId)
{
	Framework& framework = Framework::Get();
	ECS& ecs = _runtime.GetECS();

	const auto* fVital = ecs.GetStorage<FinalVital>().GetComponent(entity);
	const auto* fAttr = ecs.GetStorage<FinalAttribute>().GetComponent(entity);
	const auto* vital = ecs.GetStorage<Vital>().GetComponent(entity);
	if (!fVital || !fAttr || !vital) return;

	SendBuffer* data = NetHelper::SCStatChangePacket(
		netId,
		vital->curHp, fVital->maxHp,
		vital->curStamina, fVital->maxStamina,
		fAttr->power, fAttr->attackSpeed,
		fAttr->defense, fAttr->moveSpeed
	);

	framework.listener.SendBuffers().Enqueue(connId, data->data, data->size);
	SendBufferPool::Get().Release(data);
}
