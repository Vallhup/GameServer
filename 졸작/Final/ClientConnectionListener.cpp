#include "pch.h"
#include "ClientConnectionListener.h"
#include "Engine.h"
#include "SceneManager.h"
#include "GameScene.h"

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
	// TEMP : Server Test
	if (SceneManager* sManager = GET(Engine).GetSceneManager())
	{
		if (Scene* scene = sManager->GetCurrentScene())
		{
			if (auto testScene = dynamic_cast<GameScene*>(scene))
				testScene->HandlePacket(header, data);
		}
	}
}
