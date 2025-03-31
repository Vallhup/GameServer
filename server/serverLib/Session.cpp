#include "pch.h"

void Session::PacketProcessing(Session* session, Packet* packet)
{
	std::cout << "PacketProcessing" << std::endl;

	switch (packet->GetType()) {
	case PACKET_CONNECT:
	{
		Packet* sendPacket = new ConnectPacket(_sessionId, _pos);
		std::cout << "sendPacket session : " << sendPacket->GetSessionId();
		// 최초 Position정보 Send 후
		gServerCore->SendCall(session, sendPacket);
		break;
	}
		
	case PACKET_MOVE:
	{
		// Move 로직 처리
		MovePacket* movePacket = static_cast<MovePacket*>(packet);
		SetPosition(movePacket->GetDirection());

		// 결과 Send
		Packet* sendPacket = new MovePacket(_sessionId, _pos, true);
		ExpOver* sendOver = new ExpOver(session, sendPacket);
		DWORD sentBytes = 0;

		gServerCore->SendCall(session, sendPacket);
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
