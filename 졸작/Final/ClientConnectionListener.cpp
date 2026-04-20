#include "pch.h"
#include "ClientConnectionListener.h"
#include "Engine.h"
#include "SceneManager.h"
#include "ClientInboundPacketQueue.h"

void ClientConnectionListener::OnConnected(Connection& owner)
{
	OutputDebugStringA("OnConnected\n");
}

void ClientConnectionListener::OnDisconnected(Connection& owner)
{
	OutputDebugStringA("OnDisconnected\n");
}

void ClientConnectionListener::OnPacketReceived(
	Connection& owner, 
	const PacketHeader& header, 
	const BYTE* data)
{
	if (ClientInboundPacketQueue* inboundQueue = ENGINE.GetInboundQueue())
	{
		inboundQueue->Push(header, data);
	}
}
