#include "pch.h"
#include "ServerDirtyReplicationService.h"

#include <algorithm>
#include <span>
#include <vector>

#include "Protocol.pb.h"
#include "ECS/GameplayRuntimeComponents.h"
#include "RepComponent.h"
#include "ServerPacketStager.h"
#include "TransformHelper.h"
#include "WorldInstance.h"

void ServerDirtyReplicationService::BuildAndStage(
	FrameworkRuntime& framework,
	NetworkRuntime& network,
	SessionBindingRegistry& sessionBindings)
{
	std::vector<SessionId> worldSessionIds;
	for (WorldId worldId : framework.GetRunnableWorldIds())
	{
		WorldInstance* const world = framework.FindWorld(worldId);
		if (world == nullptr)
		{
			continue;
		}

		sessionBindings.CollectSessionsInWorld(worldId, worldSessionIds);
		if (worldSessionIds.empty())
		{
			continue;
		}

		ECSView view = world->GetRuntime().MakeView();
		for (auto [entity, dirty] : view.View<DirtyFlagsComp>())
		{
			if (!dirty.AnyDirty() || !view.HasComponent<ReplicatedTag>(entity))
			{
				continue;
			}

			const NetId netId = framework.FindNetId(worldId, entity);
			if (!netId.IsValid())
			{
				dirty.Clear();
				continue;
			}

			if (dirty.IsDirty(WorldDirtyType::Transform))
			{
				const WorldTransformComp* transform =
					view.GetComponent<WorldTransformComp>(entity);
				if (transform != nullptr)
				{
					Protocol::SC_MOVE_PACKET movePacket;
					movePacket.set_netid(netId.GetRaw());
					movePacket.set_x(transform->position.x);
					movePacket.set_y(transform->position.y);
					movePacket.set_z(transform->position.z);
					movePacket.set_yaw(
						TransformHelper::QuaternionToYaw(transform->rotation));
					(void)ServerPacketStager::StageReplicationPacket(
						network,
						PacketType::SC_MOVE_OBJECT,
						worldSessionIds,
						movePacket);
				}
			}

			if (dirty.IsDirty(WorldDirtyType::Animation))
			{
				const AnimationPlaybackStateComp* playback =
					view.GetComponent<AnimationPlaybackStateComp>(entity);
				if (playback != nullptr &&
					playback->animationId != AnimationId::None)
				{
					Protocol::SC_ANIMATION_TRANSITION_PACKET animationPacket;
					animationPacket.set_netid(netId.GetRaw());
					animationPacket.set_curranim(
						static_cast<int32_t>(playback->animationId));
					(void)ServerPacketStager::StageReplicationPacket(
						network,
						PacketType::SC_ANIMATION_CHANGE,
						worldSessionIds,
						animationPacket);
				}
			}

			if (dirty.IsDirty(WorldDirtyType::Stat))
			{
				const CombatStatStateComp* stats =
					view.GetComponent<CombatStatStateComp>(entity);
				const SessionId ownerSessionId =
					sessionBindings.FindOwnerSession(netId);
				if (stats != nullptr && ownerSessionId != 0)
				{
					Protocol::SC_STAT_CHANGE_PACKET statPacket;
					statPacket.set_netid(netId.GetRaw());
					statPacket.set_curhp(
						static_cast<uint32_t>(std::max(0, stats->currentHp)));
					statPacket.set_maxhp(
						static_cast<uint32_t>(std::max(0, stats->maxHp)));
					statPacket.set_curstamina(
						static_cast<uint32_t>(std::max(0, stats->currentStamina)));
					statPacket.set_maxstamina(
						static_cast<uint32_t>(std::max(0, stats->maxStamina)));
					statPacket.set_power(
						static_cast<uint32_t>(std::max(0, stats->attackPower)));
					statPacket.set_attackspeed(stats->attackSpeed);
					statPacket.set_defense(
						static_cast<uint32_t>(std::max(0, stats->defense)));
					statPacket.set_movespeed(
						static_cast<uint32_t>(
							std::max(0.0f, stats->moveSpeed)));
					(void)ServerPacketStager::StageReplicationPacket(
						network,
						PacketType::SC_STAT_CHANGE,
						std::span<const SessionId>(&ownerSessionId, 1),
						statPacket);
				}
			}

			dirty.Clear();
		}
	}
}
