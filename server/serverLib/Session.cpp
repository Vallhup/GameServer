#include "pch.h"
#include "Session.h"

void Session::PacketProcessing(Session* session, Packet* packet)
{
	//std::cout << "PacketProcessing" << std::endl;

	switch (packet->GetType()) {
	case PACKET_CONNECT:
	{
		Packet* sendPacket = nullptr;
		for (auto& client : gServerCore->GetSessions()) {
			if (client.first == _sessionId) {
				sendPacket = new ConnectPacket(client.first, _pos, true);
			}
			else {
				sendPacket = new ConnectPacket(client.first, _pos, false);
			}
			ExpOver* sendOver = new ExpOver(client.second.get(), sendPacket);

			gServerCore->SendCall(client.second.get(), sendPacket);
		}

		break;
	}
		
	case PACKET_MOVE:
	{
		// Move 로직 처리
		MovePacket* movePacket = static_cast<MovePacket*>(packet);
		SetPosition(movePacket->GetDirection());

		// 결과 Send
		// 접속해있는 모든 Client에게 Broadcast
		// but MovePacket을 보낸 Client에게는 true
		// 나머지 Client에게는 false
		Packet* sendPacket = nullptr;
		for (auto& client : gServerCore->GetSessions()) {
			if (client.first == _sessionId) {
				sendPacket = new MovePacket(client.first, _pos, true);
			}
			else {
				sendPacket = new MovePacket(client.first, _pos, false);
			}
			ExpOver* sendOver = new ExpOver(client.second.get(), sendPacket);
			
			gServerCore->SendCall(client.second.get(), sendPacket);
		}

		break;
	}
		

	case PACKET_DISCONNECT:


		break;

	}
}

void Session::SetPosition(int moveDirection)
{
	switch (moveDirection) {
	case MoveDirection::UP:
		if (--_pos._yPos < 1) _pos._yPos = 1;
		break;

	case MoveDirection::DOWN:
		if (++_pos._yPos > 8) _pos._yPos = 8;
		break;

	case MoveDirection::LEFT:
		if (--_pos._xPos < 1) _pos._xPos = 1;
		break;

	case MoveDirection::RIGHT:
		if (++_pos._xPos > 8) _pos._xPos = 8;
		break;
	}
}
