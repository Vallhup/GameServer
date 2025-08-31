#include "pch.h"
#include "PacketFactory.h"

std::vector<char> PacketFactory::CSLoginPacket()
{
	Protocol::CS_LOGIN_PACKET login;

	std::string body;
	login.SerializeToString(&body);

	Protocol::GamePacket game;
	game.mutable_header()->set_type(Protocol::PacketType::CS_LOGIN);
	game.mutable_header()->set_sessionid(-1);
	game.set_body(body);

	std::string gameString;
	game.SerializeToString(&gameString);

	const uint16_t payloadSize = static_cast<uint16_t>(gameString.size());

	std::vector<char> out(sizeof(payloadSize) + payloadSize);
	memcpy(out.data(), &payloadSize, sizeof(payloadSize));
	memcpy(out.data() + sizeof(payloadSize), gameString.data(), gameString.size());

	return out;
}

std::vector<char> PacketFactory::CSInputPacket(Protocol::Input key, Protocol::InputType type)
{
	Protocol::CS_INPUT_PACKET input;
	
	input.set_key(key);
	input.set_inputtype(type);

	std::string body;
	input.SerializeToString(&body);

	Protocol::GamePacket game;
	game.mutable_header()->set_type(Protocol::PacketType::CS_INPUT);

	// TEMP : 나중에 Client에서 제대로 된 자신의 ID Setting
	game.mutable_header()->set_sessionid(1);
	game.set_body(body);

	std::string gameString;
	game.SerializeToString(&gameString);

	const uint16_t payloadSize = static_cast<uint16_t>(gameString.size());

	std::vector<char> out(sizeof(payloadSize) + payloadSize);
	memcpy(out.data(), &payloadSize, sizeof(payloadSize));
	memcpy(out.data() + sizeof(payloadSize), gameString.data(), gameString.size());

	return out;
}

std::vector<char> PacketFactory::SCLoginPacket(int id)
{
	Protocol::SC_LOGIN_PACKET login;

	std::string body;
	login.SerializeToString(&body);

	Protocol::GamePacket game;
	game.mutable_header()->set_type(Protocol::PacketType::SC_LOGIN);
	game.mutable_header()->set_sessionid(id);
	game.set_body(body);

	std::string gameString;
	game.SerializeToString(&gameString);

	const uint16_t payloadSize = static_cast<uint16_t>(gameString.size());

	std::vector<char> out(sizeof(payloadSize) + payloadSize);
	memcpy(out.data(), &payloadSize, sizeof(payloadSize));
	memcpy(out.data() + sizeof(payloadSize), gameString.data(), gameString.size());

	return out;
}

std::vector<char> PacketFactory::SCAddPacket(int id, const Protocol::Vec3& pos)
{
	Protocol::SC_ADD_PACKET add;
	add.mutable_pos()->set_x(pos.x());
	add.mutable_pos()->set_y(pos.y());
	add.mutable_pos()->set_z(pos.z());

	std::string body;
	add.SerializeToString(&body);

	Protocol::GamePacket game;
	game.mutable_header()->set_type(Protocol::PacketType::SC_ADD);
	game.mutable_header()->set_sessionid(id);
	game.set_body(body);

	std::string gameString;
	game.SerializeToString(&gameString);

	const uint16_t payloadSize = static_cast<uint16_t>(gameString.size());

	std::vector<char> out(sizeof(payloadSize) + payloadSize);
	memcpy(out.data(), &payloadSize, sizeof(payloadSize));
	memcpy(out.data() + sizeof(payloadSize), gameString.data(), gameString.size());

	return out;
}

std::vector<char> PacketFactory::SCMovePakcet(int id, const Protocol::Vec3& pos)
{
	Protocol::SC_MOVE_PACKET move;
	move.mutable_pos()->set_x(pos.x());
	move.mutable_pos()->set_y(pos.y());
	move.mutable_pos()->set_z(pos.z());

	std::string body;
	move.SerializeToString(&body);

	Protocol::GamePacket game;
	game.mutable_header()->set_type(Protocol::PacketType::SC_MOVE_OBJECT);
	game.mutable_header()->set_sessionid(id);
	game.set_body(body);

	std::string gameString;
	game.SerializeToString(&gameString);

	const uint16_t payloadSize = static_cast<uint16_t>(gameString.size());

	std::vector<char> out(sizeof(payloadSize) + payloadSize);
	memcpy(out.data(), &payloadSize, sizeof(payloadSize));
	memcpy(out.data() + sizeof(payloadSize), gameString.data(), gameString.size());

	return out;
}

std::vector<char> PacketFactory::SCRemovePacket(int id)
{
	Protocol::SC_REMOVE_PACKET remove;

	std::string body;
	remove.SerializeToString(&body);

	Protocol::GamePacket game;
	game.mutable_header()->set_type(Protocol::PacketType::SC_REMOVE);
	game.mutable_header()->set_sessionid(id);
	game.set_body(body);

	std::string gameString;
	game.SerializeToString(&gameString);

	const uint16_t payloadSize = static_cast<uint16_t>(gameString.size());

	std::vector<char> out(sizeof(payloadSize) + payloadSize);
	memcpy(out.data(), &payloadSize, sizeof(payloadSize));
	memcpy(out.data() + sizeof(payloadSize), gameString.data(), gameString.size());

	return out;
}
