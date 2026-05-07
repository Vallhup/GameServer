#pragma once

#include <cstdint>
#include <span>

#include "CharacterDef.h"
#include "NetworkRuntime.h"
#include "PacketFactory.h"
#include "Protocol.pb.h"
#include "Session.h"
#include "ECS/GameplayRuntimeComponents.h"
#include "TransformHelper.h"

struct ServerWorldTransitionBeginPacket
{
	uint64_t transferId{ 0 };
	uint32_t requestId{ 0 };
	uint32_t sourceWorldDefId{ 0 };
	uint64_t sourceWorldId{ 0 };
	uint32_t targetWorldDefId{ 0 };
	uint64_t targetWorldId{ 0 };
	uint32_t mapResourceId{ 0 };
	uint64_t playerNetId{ 0 };
	bool clearExistingObjects{ true };
	bool waitClientReady{ true };
	bool usedFallback{ false };
	uint32_t reason{ 0 };
};

class ServerPacketStager final {
public:
	static bool StageLoginSuccess(
		NetworkRuntime& network,
		SessionId sessionId,
		NetId playerNetId);

	static bool StageLoginFail(
		NetworkRuntime& network,
		SessionId sessionId,
		uint32_t reason);

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

	static bool StageWorldTransitionBeginPacket(
		NetworkRuntime& network,
		SessionId sessionId,
		const ServerWorldTransitionBeginPacket& transition);

	static bool StageWorldTransitionRejectedPacket(
		NetworkRuntime& network,
		SessionId sessionId,
		uint32_t requestId,
		uint32_t reason);

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
