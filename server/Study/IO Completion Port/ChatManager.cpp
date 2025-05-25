#include "pch.h"
#include "ChatManager.h"

void ChatManager::HandleMessage(short senderId, const char* msg)
{
	auto service = _service.lock();
	if (nullptr == service) {
		return;
	}

	std::string text(msg);

	// 1. 보내려는 message의 길이 검사 / 제한 (CHAT_SIZE)
	if (text.size() >= sizeof(CHAT_SIZE)) {
		text.resize(CHAT_SIZE - 1);
	}

	// 2. Packet Broadcast
	SC_CHAT_PACKET chat;
	chat.id = senderId;
	chat.size = sizeof(chat);
	chat.type = SC_CHAT;
	strcpy_s(chat.message, CHAT_SIZE - 1, text.c_str());

	service->Broadcast(PacketFactory::Serialize(chat));
}
