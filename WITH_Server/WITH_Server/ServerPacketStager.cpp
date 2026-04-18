#include "pch.h"
#include "ServerPacketStager.h"

bool ServerPacketStager::StageLoginResponse(
	NetworkRuntime& network,
	SessionId sessionId,
	NetId playerNetId)
{
	Protocol::SC_LOGIN_PACKET loginAck;
	loginAck.set_netid(playerNetId.GetRaw());

	SendBuffer* const buffer =
		PacketFactory::Serialize(PacketType::SC_LOGIN, loginAck);
	if (buffer == nullptr)
	{
		return false;
	}

	const bool staged =
		network.StageUnicast(
			sessionId,
			std::span<const uint8_t>(buffer->data, buffer->size));

	SendBufferPool::Get().Release(buffer);
	return staged;
}

bool ServerPacketStager::StageSpawnAddPacketToSession(
	NetworkRuntime& network,
	SessionId sessionId,
	NetId netId,
	CharacterId characterId,
	const WorldTransformComp* transform)
{
	Protocol::SC_ADD_PACKET add;
	add.set_netid(netId.GetRaw());
	add.set_typeid_(static_cast<int>(characterId));
	add.set_x(transform != nullptr ? transform->position.x : 0.0f);
	add.set_y(transform != nullptr ? transform->position.y : 0.0f);
	add.set_z(transform != nullptr ? transform->position.z : 0.0f);
	add.set_yaw(transform != nullptr ?
		TransformHelper::QuaternionToYaw(transform->rotation) : 0.0f);

	SendBuffer* const buffer =
		PacketFactory::Serialize(PacketType::SC_ADD, add);
	if (buffer == nullptr)
	{
		return false;
	}

	const bool staged =
		network.StageUnicast(
			sessionId,
			std::span<const uint8_t>(buffer->data, buffer->size));
	SendBufferPool::Get().Release(buffer);
	return staged;
}

bool ServerPacketStager::StageSpawnAddPacketToSessions(
	NetworkRuntime& network,
	std::span<const SessionId> sessionIds,
	NetId netId,
	CharacterId characterId,
	const WorldTransformComp* transform)
{
	if (sessionIds.empty())
	{
		return true;
	}

	Protocol::SC_ADD_PACKET add;
	add.set_netid(netId.GetRaw());
	add.set_typeid_(static_cast<int>(characterId));
	add.set_x(transform != nullptr ? transform->position.x : 0.0f);
	add.set_y(transform != nullptr ? transform->position.y : 0.0f);
	add.set_z(transform != nullptr ? transform->position.z : 0.0f);
	add.set_yaw(transform != nullptr ?
		TransformHelper::QuaternionToYaw(transform->rotation) : 0.0f);

	SendBuffer* const buffer =
		PacketFactory::Serialize(PacketType::SC_ADD, add);
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

bool ServerPacketStager::StageSpawnRemovePacketToSessions(
	NetworkRuntime& network,
	std::span<const SessionId> sessionIds,
	NetId netId)
{
	if (sessionIds.empty())
	{
		return true;
	}

	Protocol::SC_REMOVE_PACKET remove;
	remove.set_netid(netId.GetRaw());

	SendBuffer* const buffer =
		PacketFactory::Serialize(PacketType::SC_REMOVE, remove);
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

bool ServerPacketStager::StageWorldTransitionBeginPacket(
	NetworkRuntime& network,
	SessionId sessionId,
	const ServerWorldTransitionBeginPacket& transition)
{
	Protocol::SC_WORLD_TRANSITION_BEGIN_PACKET packet;
	packet.set_transferid(transition.transferId);
	packet.set_requestid(transition.requestId);
	packet.set_sourceworlddefid(transition.sourceWorldDefId);
	packet.set_sourceworldid(transition.sourceWorldId);
	packet.set_targetworlddefid(transition.targetWorldDefId);
	packet.set_targetworldid(transition.targetWorldId);
	packet.set_mapresourceid(transition.mapResourceId);
	packet.set_playernetid(transition.playerNetId);
	packet.set_clearexistingobjects(transition.clearExistingObjects);
	packet.set_waitclientready(transition.waitClientReady);
	packet.set_usedfallback(transition.usedFallback);
	packet.set_reason(transition.reason);

	SendBuffer* const buffer =
		PacketFactory::Serialize(PacketType::SC_WORLD_TRANSITION_BEGIN, packet);
	if (buffer == nullptr)
	{
		return false;
	}

	const bool staged =
		network.StageUnicast(
			sessionId,
			std::span<const uint8_t>(buffer->data, buffer->size));
	SendBufferPool::Get().Release(buffer);
	return staged;
}

bool ServerPacketStager::StageWorldTransitionRejectedPacket(
	NetworkRuntime& network,
	SessionId sessionId,
	uint32_t requestId,
	uint32_t reason)
{
	Protocol::SC_WORLD_TRANSITION_REJECTED_PACKET packet;
	packet.set_requestid(requestId);
	packet.set_reason(reason);

	SendBuffer* const buffer =
		PacketFactory::Serialize(PacketType::SC_WORLD_TRANSITION_REJECTED, packet);
	if (buffer == nullptr)
	{
		return false;
	}

	const bool staged =
		network.StageUnicast(
			sessionId,
			std::span<const uint8_t>(buffer->data, buffer->size));
	SendBufferPool::Get().Release(buffer);
	return staged;
}
