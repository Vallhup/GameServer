#include "pch.h"
#include "ClientPacketRouter.h"
#include "ClientInboundPacketQueue.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Engine.h"
#include "NetHelper.h"
#include "ClientWorldTransitionController.h"

void ClientPacketRouter::Route(const ClientInboundPacket& packet)
{
	// TODO: 기존 packet들은 그대로 Scene으로 보내서 처리
	//       WorldTransition packet들은 ClientWorldTransitionController에 저장
	const PacketType type = static_cast<PacketType>(packet.header.type);
	switch (type) {
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
