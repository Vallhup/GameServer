#include "pch.h"
#include "ChatManager.h"

void ChatManager::HandleMessage(short senderId, const char* msg)
{
	std::string text(msg);

	// 1. 보내려는 message의 길이 검사 / 제한 (BUF_SIZE)
	if (text.size() >= sizeof(BUF_SIZE)) {
		text.resize(BUF_SIZE - 1);
	}

	// 2. Packet Broadcast
	SC_CHAT_BROADCAST_PACKET chat;
	chat.id = senderId;
	chat.size = sizeof(chat);
	chat.type = SC_CHAT_BROADCAST;
	strcpy_s(chat.message, BUF_SIZE - 1, text.c_str());

	_service.Broadcast(Serialize(chat));
}
