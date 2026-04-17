#pragma once

#include <span>

#include "CharacterDef.h"
#include "NetworkRuntime.h"
#include "PacketFactory.h"
#include "Protocol.pb.h"
#include "Session.h"
#include "ECS/GameplayRuntimeComponents.h"
#include "TransformHelper.h"

class ServerPacketStager final {
public:
	static bool StageLoginResponse(
		NetworkRuntime& network,
		SessionId sessionId,
		NetId playerNetId);

	static bool StageSpawnAddPacketToSession(
		NetworkRuntime& network,
		SessionId sessionId,
		NetId netId,
		CharacterId characterId,
		const WorldTransformComp* transform = nullptr);

	static bool StageSpawnAddPacketToSessions(
		NetworkRuntime& network,
		std::span<const SessionId> sessionIds,
		NetId netId,
		CharacterId characterId,
		const WorldTransformComp* transform = nullptr);

	static bool StageSpawnRemovePacketToSessions(
		NetworkRuntime& network,
		std::span<const SessionId> sessionIds,
		NetId netId);

	template<typename TPacket>
	static bool StageReplicationPacket(
		NetworkRuntime& network,
		PacketType packetType,
		std::span<const SessionId> sessionIds,
		const TPacket& packet)
	{
		if (sessionIds.empty())
		{
			return true;
		}

		SendBuffer* const buffer =
			PacketFactory::Serialize(packetType, packet);
		if (buffer == nullptr)
		{
			return false;
		}

		const bool staged = network.StageMulticast(
			sessionIds,
			std::span<const uint8_t>(buffer->data, buffer->size));
		SendBufferPool::Get().Release(buffer);
		return staged;
	}
};
