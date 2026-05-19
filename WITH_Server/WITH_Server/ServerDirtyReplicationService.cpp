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
	SessionFlowController& sessionFlow,
	std::span<const SessionId> excludedSessionIds)
{
	std::vector<SessionId> worldSessionIds;
	for (WorldId worldId : framework.GetRunnableWorldIds())
	{
		WorldInstance* const world = framework.FindWorld(worldId);
		if (world == nullptr)
		{
			continue;
		}

		sessionFlow.CollectSessionsInWorld(worldId, worldSessionIds);
		if (!excludedSessionIds.empty())
		{
			worldSessionIds.erase(
				std::remove_if(
					worldSessionIds.begin(),
					worldSessionIds.end(),
					[excludedSessionIds](SessionId sessionId)
					{
						return std::find(
							excludedSessionIds.begin(),
							excludedSessionIds.end(),
							sessionId) != excludedSessionIds.end();
					}),
				worldSessionIds.end());
		}
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
					animationPacket.set_abilityinstanceid(
						playback->boundAbilityInstanceId);
					animationPacket.set_normalizedtime(
						playback->normalizedTime);
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
				if (stats != nullptr)
				{
					(void)ServerPacketStager::StageStatPacketToSessions(
						network,
						worldSessionIds,
						netId,
						*stats);
				}
			}

			dirty.Clear();
		}
	}
}
