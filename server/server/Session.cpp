#include "Session.h"
#include "ServerCore.h"
#include "ExpOver.h"
#include "Packet.h"

void Session::PacketProcessing(Session* session, Packet& packet)
{
	switch (packet.GetType()) {
	case PACKET_MOVE:
	{
		// Move 로직 처리
		MovePacket* movePacket = static_cast<MovePacket*>(&packet);
		SetPosition(movePacket->GetDirection());

		// 결과 Send
		Packet* sendPacket = new MovePacket(_sessionId, _pos, true);
		ExpOver* sendOver = new ExpOver(session, *sendPacket);
		DWORD sentBytes = 0;

		WSASend(_socket, sendOver->GetWsabuf(), 1, &sentBytes, 0, sendOver->GetOverPtr(), gServerCore->SendCallback);

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
		if (++_pos._yPos > 8) _pos._yPos = 8;
		break;

	case MoveDirection::DOWN:
		if (--_pos._yPos < 1) _pos._yPos = 1;
		break;

	case MoveDirection::LEFT:
		if (--_pos._xPos < 1) _pos._xPos = 1;
		break;

	case MoveDirection::RIGHT:
		if (++_pos._xPos > 8) _pos._xPos = 8;
		break;
	}
}
