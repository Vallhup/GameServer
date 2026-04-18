#include "pch.h"
#include "GameServerNetworkListener.h"

#include "Connection.h"
#include "PacketFactory.h"
#include "PacketType.h"

namespace
{
	constexpr uint16_t kProtocolErrorDecodeFailed = 1;
	constexpr uint16_t kProtocolErrorUnknownPacket = 2;

	void PublishProtocolError(
		IInboundMessageSink& sink,
		SessionId sessionId,
		uint16_t packetId,
		uint16_t reason)
	{
		InboundMessage message{};
		message.sessionId = sessionId;
		message.kind = InboundMessageKind::ProtocolError;
		message.packetId = packetId;
		message.payload.error.packetId = packetId;
		message.payload.error.reason = reason;
		sink.OnInboundMessage(std::move(message));
	}
}

GameServerNetworkListener::GameServerNetworkListener(IInboundMessageSink& sink)
	: _sink(sink)
{
}

void GameServerNetworkListener::OnConnected(Connection& owner)
{
	InboundMessage message{};
	message.kind = InboundMessageKind::Connected;
	message.sessionId = owner.GetId();

	_sink.OnInboundMessage(std::move(message));
}

void GameServerNetworkListener::OnDisconnected(Connection& owner)
{
	InboundMessage message{};
	message.kind = InboundMessageKind::Disconnected;
	message.sessionId = owner.GetId();

	_sink.OnInboundMessage(std::move(message));
}

void GameServerNetworkListener::OnPacketReceived(
	Connection& owner,
	const PacketHeader& header,
	const BYTE* data)
{
	const SessionId sessionId = owner.GetId();
	const PacketType packetType = static_cast<PacketType>(header.type);

	InboundMessage message{};
	message.sessionId = sessionId;
	message.packetId = header.type;

	switch (packetType) {
	case PacketType::CS_LOGIN:
	{
		Protocol::CS_LOGIN_PACKET packet;
		if (!PacketFactory::Deserialize(header, data, &packet))
		{
			PublishProtocolError(
				_sink,
				sessionId,
				header.type,
				kProtocolErrorDecodeFailed);
			return;
		}

		message.kind = InboundMessageKind::LoginPacket;
		_sink.OnInboundMessage(std::move(message));
		return;
	}

	case PacketType::CS_MOVE:
	{
		Protocol::CS_MOVE_PACKET packet;
		if (!PacketFactory::Deserialize(header, data, &packet))
		{
			PublishProtocolError(
				_sink,
				sessionId,
				header.type,
				kProtocolErrorDecodeFailed);
			return;
		}

		message.kind = InboundMessageKind::MovePacket;
		message.payload.move.inputX = packet.inputx();
		message.payload.move.inputZ = packet.inputz();
		message.payload.move.yaw = packet.yaw();
		message.payload.move.isRun = packet.isrun();
		_sink.OnInboundMessage(std::move(message));
		return;
	}

	case PacketType::CS_ATTACK:
	{
		Protocol::CS_ATTACK_PACKET packet;
		if (!PacketFactory::Deserialize(header, data, &packet))
		{
			PublishProtocolError(
				_sink,
				sessionId,
				header.type,
				kProtocolErrorDecodeFailed);
			return;
		}

		message.kind = InboundMessageKind::AttackPacket;
		message.payload.direction.dirX = packet.dirx();
		message.payload.direction.dirZ = packet.dirz();
		_sink.OnInboundMessage(std::move(message));
		return;
	}

	case PacketType::CS_DODGE:
	{
		Protocol::CS_DODGE_PACKET packet;
		if (!PacketFactory::Deserialize(header, data, &packet))
		{
			PublishProtocolError(
				_sink,
				sessionId,
				header.type,
				kProtocolErrorDecodeFailed);
			return;
		}

		message.kind = InboundMessageKind::DodgePacket;
		message.payload.direction.dirX = packet.dirx();
		message.payload.direction.dirZ = packet.dirz();
		_sink.OnInboundMessage(std::move(message));
		return;
	}

	case PacketType::CS_GUARD:
	{
		Protocol::CS_GUARD_PACKET packet;
		if (!PacketFactory::Deserialize(header, data, &packet))
		{
			PublishProtocolError(
				_sink,
				sessionId,
				header.type,
				kProtocolErrorDecodeFailed);
			return;
		}

		message.kind = InboundMessageKind::GuardPacket;
		message.payload.guard.input = packet.input();
		_sink.OnInboundMessage(std::move(message));
		return;
	}

	case PacketType::CS_PARRY:
	{
		Protocol::CS_PARRY_PACKET packet;
		if (!PacketFactory::Deserialize(header, data, &packet))
		{
			PublishProtocolError(
				_sink,
				sessionId,
				header.type,
				kProtocolErrorDecodeFailed);
			return;
		}

		message.kind = InboundMessageKind::ParryPacket;
		message.payload.direction.dirX = packet.dirx();
		message.payload.direction.dirZ = packet.dirz();
		_sink.OnInboundMessage(std::move(message));
		return;
	}

	case PacketType::CS_WORLD_TRANSITION_REQUEST:
	{
		Protocol::CS_WORLD_TRANSITION_REQUEST_PACKET packet;
		if (!PacketFactory::Deserialize(header, data, &packet))
		{
			PublishProtocolError(
				_sink,
				sessionId,
				header.type,
				kProtocolErrorDecodeFailed);
			return;
		}

		message.kind = InboundMessageKind::WorldTransitionRequestPacket;
		message.payload.worldTransitionRequest.requestId = packet.requestid();
		_sink.OnInboundMessage(std::move(message));
		return;
	}

	case PacketType::CS_WORLD_TRANSITION_READY:
	{
		Protocol::CS_WORLD_TRANSITION_READY_PACKET packet;
		if (!PacketFactory::Deserialize(header, data, &packet))
		{
			PublishProtocolError(
				_sink,
				sessionId,
				header.type,
				kProtocolErrorDecodeFailed);
			return;
		}

		message.kind = InboundMessageKind::WorldTransitionReadyPacket;
		message.payload.worldTransitionReady.transferId = packet.transferid();
		_sink.OnInboundMessage(std::move(message));
		return;
	}

	default:
		PublishProtocolError(
			_sink,
			sessionId,
			header.type,
			kProtocolErrorUnknownPacket);
		return;
	}
}
