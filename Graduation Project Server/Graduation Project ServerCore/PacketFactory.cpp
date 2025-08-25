#include "pch.h"
#include "PacketFactory.h"

std::vector<char> PacketFactory::CSInputPacket(Protocol::Input key, Protocol::InputType type)
{
	Protocol::CS_INPUT_PACKET input;
	input.mutable_header()->set_type(Protocol::CS_INPUT);

	// TEMP : 나중에 Client에서 제대로 된 자신의 ID Setting
	input.mutable_header()->set_sessionid(0);
	input.set_key(key);
	input.set_inputtype(type);

	std::string body;
	input.SerializeToString(&body);

	const uint16_t size = static_cast<uint16_t>(body.size());

	std::vector<char> out(sizeof(size) + body.size());
	memcpy(out.data(), &size, sizeof(size));
	memcpy(out.data() + sizeof(size), body.data(), body.size());

	return out;
}
