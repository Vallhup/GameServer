#include "pch.h"
#include "ClientPacketRouter.h"
#include "ClientInboundPacketQueue.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Engine.h"
#include "NetHelper.h"
#include "ClientWorldTransitionController.h"
#include "ClientPartyState.h"
#include "ClientTitleState.h"
#include "NetworkManager.h"

void ClientPacketRouter::Route(const ClientInboundPacket& packet)
{
	switch (static_cast<PacketType>(packet.header.type)) {
	case PacketType::SC_TIME_SYNC:
	{
		HandleTimeSync(packet);
		break;
	}
	case PacketType::SC_WORLD_TRANSITION_BEGIN:
	{
		HandleWorldTransitionBegin(packet);
		break;
	}
	case PacketType::SC_WORLD_TRANSITION_REJECTED:
	{
		HandleWorldTransitionRejected(packet);
		break;
	}
	case PacketType::SC_PARTY_UI_BOOTSTRAP:
	{
		HandlePartyUiBootstrap(packet);
		break;
	}
	case PacketType::SC_PARTY_LIST_SNAPSHOT:
	{
		HandlePartyListSnapshot(packet);
		break;
	}
	case PacketType::SC_PARTY_COMMAND_RESULT:
	{
		HandlePartyCommandResult(packet);
		break;
	}
	case PacketType::SC_PARTY_SNAPSHOT:
	{
		HandlePartySnapshot(packet);
		break;
	}
	case PacketType::SC_PARTY_JOIN_REQUEST_RECEIVED:
	{
		HandlePartyJoinRequestReceived(packet);
		break;
	}
	case PacketType::SC_PARTY_JOIN_REQUEST_CLOSED:
	{
		HandlePartyJoinRequestClosed(packet);
		break;
	}
	case PacketType::SC_STAT_UI_BOOTSTRAP:
	{
		HandleStatUiBootstrap(packet);
		break;
	}
	case PacketType::SC_TITLE_EQUIP_RESULT:
	{
		HandleTitleEquipResult(packet);
		break;
	}
	default:
	{
		RouteToCurrentScene(packet);
		break;
	}
	}
}

void ClientPacketRouter::RouteToCurrentScene(const ClientInboundPacket& packet)
{
	if (SceneManager* sManager = SCENE_MANAGER)
	{
		if (Scene* scene = sManager->GetCurrentScene())
		{
			scene->HandlePacket(packet.header, packet.bytes.data());
		}
	}
}

void ClientPacketRouter::HandleTimeSync(const ClientInboundPacket& packet)
{
	NetHelper::DispatchPacket<Protocol::SC_TIME_SYNC_PACKET>(
		packet.header, packet.bytes.data(),
		[this](const auto& packet)
		{
			if (NetworkManager* nManager = NETWORK_MANAGER)
			{
				nManager->SendTimeSyncPacket(
					packet.probeseq(), packet.serversendtimems());
			}
		}
	);
}

void ClientPacketRouter::HandleWorldTransitionBegin(const ClientInboundPacket& packet)
{
	NetHelper::DispatchPacket<Protocol::SC_WORLD_TRANSITION_BEGIN_PACKET>(
		packet.header, packet.bytes.data(),
		[this](const auto& packet) 
		{ 
			const bool accepted = 
				ENGINE.GetWorldTransitionController().OnBegin(packet);

			if (false == accepted)
			{
				OutputDebugStringA("WorldTransition Begin ignored\n");
			}
		}
	);
}

void ClientPacketRouter::HandleWorldTransitionRejected(const ClientInboundPacket& packet)
{
	NetHelper::DispatchPacket<Protocol::SC_WORLD_TRANSITION_REJECTED_PACKET>(
		packet.header, packet.bytes.data(),
		[this](const auto& packet)
		{
			const bool accepted =
				ENGINE.GetWorldTransitionController().OnRejected(packet);

			if (false == accepted)
			{
				OutputDebugStringA("WorldTransition Rejected ignored\n");
			}
		}
	);
}

void ClientPacketRouter::HandlePartyUiBootstrap(const ClientInboundPacket& packet)
{
	NetHelper::DispatchPacket<Protocol::SC_PARTY_UI_BOOTSTRAP_PACKET>(
		packet.header, packet.bytes.data(),
		[](const auto& packet)
		{
			if (ClientPartyState* partyState = ENGINE.GetPartyState())
			{
				partyState->ApplyUIBootstrap(packet);
			}
		}
	);
}

void ClientPacketRouter::HandlePartyListSnapshot(const ClientInboundPacket& packet)
{
	NetHelper::DispatchPacket<Protocol::SC_PARTY_LIST_SNAPSHOT_PACKET>(
		packet.header, packet.bytes.data(),
		[](const auto& packet)
		{
			if (ClientPartyState* partyState = ENGINE.GetPartyState())
			{
				partyState->ApplyListSnapshot(packet);
			}
		}
	);
}

void ClientPacketRouter::HandlePartyCommandResult(const ClientInboundPacket& packet)
{
	NetHelper::DispatchPacket<Protocol::SC_PARTY_COMMAND_RESULT_PACKET>(
		packet.header, packet.bytes.data(),
		[](const auto& packet)
		{
			if (ClientPartyState* partyState = ENGINE.GetPartyState())
			{
				partyState->ApplyCommandResult(packet);
			}
		}
	);
}

void ClientPacketRouter::HandlePartySnapshot(const ClientInboundPacket& packet)
{
	NetHelper::DispatchPacket<Protocol::SC_PARTY_SNAPSHOT_PACKET>(
		packet.header, packet.bytes.data(),
		[](const auto& packet)
		{
			if (ClientPartyState* partyState = ENGINE.GetPartyState())
			{
				partyState->ApplySnapshot(packet);
			}
		}
	);
}

void ClientPacketRouter::HandlePartyJoinRequestReceived(const ClientInboundPacket& packet)
{
	NetHelper::DispatchPacket<Protocol::SC_PARTY_JOIN_REQUEST_RECEIVED_PACKET>(
		packet.header, packet.bytes.data(),
		[](const auto& packet)
		{
			if (ClientPartyState* partyState = ENGINE.GetPartyState())
			{
				partyState->ApplyJoinRequestReceived(packet);
			}
		}
	);
}

void ClientPacketRouter::HandlePartyJoinRequestClosed(const ClientInboundPacket& packet)
{
	NetHelper::DispatchPacket<Protocol::SC_PARTY_JOIN_REQUEST_CLOSED_PACKET>(
		packet.header, packet.bytes.data(),
		[](const auto& packet)
		{
			if (ClientPartyState* partyState = ENGINE.GetPartyState())
			{
				partyState->ApplyJoinRequestClosed(packet);
			}
		}
	);
}

void ClientPacketRouter::HandleStatUiBootstrap(
	const ClientInboundPacket& packet)
{
	NetHelper::DispatchPacket<Protocol::SC_STAT_UI_BOOTSTRAP_PACKET>(
		packet.header,
		packet.bytes.data(),
		[](const auto& message)
		{
			if (ClientTitleState* titleState = ENGINE.GetTitleState())
			{
				titleState->ApplyBootstrap(message);
			}
		});
}

void ClientPacketRouter::HandleTitleEquipResult(
	const ClientInboundPacket& packet)
{
	NetHelper::DispatchPacket<Protocol::SC_TITLE_EQUIP_RESULT_PACKET>(
		packet.header,
		packet.bytes.data(),
		[](const auto& message)
		{
			if (ClientTitleState* titleState = ENGINE.GetTitleState())
			{
				titleState->ApplyEquipResult(message);
			}
		});
}
