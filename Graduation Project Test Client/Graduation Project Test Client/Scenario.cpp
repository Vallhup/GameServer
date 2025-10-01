#include "Scenario.h"

#include "../../Graduation Project Server/Graduation Project ServerCore/Protocols/Enum.pb.h"
#include "../../Graduation Project Server/Graduation Project ServerCore/Protocols/Struct.pb.h"
#include "../../Graduation Project Server/Graduation Project ServerCore/Protocols/Protocol.pb.h"

void ConnectScenario::OnStart(Client* client)
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

	std::vector<char> packet(sizeof(payloadSize) + payloadSize);
	memcpy(packet.data(), &payloadSize, sizeof(payloadSize));
	memcpy(packet.data() + sizeof(payloadSize), gameString.data(), gameString.size());

	client->RegisterSend(packet);
}

void ConnectScenario::OnTick(Client* client)
{

}

void ConnectScenario::OnPacket(Client* client, const std::vector<char>& packet)
{
}
