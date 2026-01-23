#pragma once

#include "Event.h"

class NetworkHandler {
	using HandlerFunc = bool(*)(uint32, const PacketHeader&, const BYTE*);

public:
	NetworkHandler();

	bool Handle(uint32 id, const PacketHeader& header, const BYTE* data)
	{
		auto it = _handlerTable.find(header.type);
		if (it != _handlerTable.end())
			return it->second(id, header, data);

		return false;
	}

private:
	// TODO : Handler 함수 추가
	static bool HandleConnect(uint32 id, const PacketHeader& header, const BYTE* data);
	static bool HandleMove(uint32 id, const PacketHeader& header, const BYTE* data);
	static bool HandleAttack(uint32 id, const PacketHeader& header, const BYTE* data);
	static bool HandleDodge(uint32 id, const PacketHeader& header, const BYTE* data);
	static bool HandleGuard(uint32 id, const PacketHeader& header, const BYTE* data);
	static bool HandleParry(uint32 id, const PacketHeader& header, const BYTE* data);

	std::unordered_map<uint16, HandlerFunc> _handlerTable;
};
