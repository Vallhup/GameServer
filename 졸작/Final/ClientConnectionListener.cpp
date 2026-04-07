#include "pch.h"
#include "ClientConnectionListener.h"
#include "Engine.h"
#include "SceneManager.h"
#include "FirstBattleScene.h"

void ClientConnectionListener::OnConnected(Connection& owner)
{
	OutputDebugStringA("OnConnected\n");
}

void ClientConnectionListener::OnDisconnected(Connection& owner)
{
	OutputDebugStringA("OnDisconnected\n");
}

void ClientConnectionListener::OnPacketReceived(Connection& owner, const PacketHeader& header, const BYTE* data)
{
	if (SceneManager* sManager = SCENE_MANAGER)
	{
		if (Scene* scene = sManager->GetCurrentScene())
			scene->HandlePacket(header, data);
	}
}
