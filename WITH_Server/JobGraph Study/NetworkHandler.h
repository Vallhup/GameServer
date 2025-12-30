#pragma once

#include <unordered_map>

#include "Protocol.hpp"
#include "Event.h"

class NetworkHandler {
	using HandlerFunc = bool(*)(int, const PacketHeader&, const char*);

public:
	static NetworkHandler& Get()
	{
		static NetworkHandler converter;
		return converter;
	}

	bool Handle(int id, const PacketHeader& header, const char* data)
	{
		auto it = _handlerTable.find(header.type);
		if (it != _handlerTable.end())
			return it->second(id, header, data);

		return false;
	}

private:
	NetworkHandler()
	{
		_handlerTable[(uint16_t)PacketType::CS_LOGIN] = &NetworkHandler::HandleConnect;
		_handlerTable[(uint16_t)PacketType::CS_MOVE] = &NetworkHandler::HandleMove;
	}

	// TODO : Handler 함수 추가
	static bool HandleConnect(int id, const PacketHeader& header, const char* data);
	static bool HandleMove(int id, const PacketHeader& header, const char* data);

	std::unordered_map<uint16_t, HandlerFunc> _handlerTable;
};
