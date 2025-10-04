#include "Scenario.h"

#include "../../Graduation Project Server/Graduation Project ServerCore/Protocols/Enum.pb.h"
#include "../../Graduation Project Server/Graduation Project ServerCore/Protocols/Struct.pb.h"
#include "../../Graduation Project Server/Graduation Project ServerCore/Protocols/Protocol.pb.h"
#include "../../Graduation Project Server/Graduation Project ServerCore/PacketFactory.h"

/*---------------[ ConnectScenario ]---------------*/

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

/*---------------[ MoveScenario ]---------------*/

void MoveScenario::OnStart(Client* client)
{

}

void MoveScenario::OnTick(Client* client)
{
	using namespace std::chrono;

	const int id = client->GetId();
	static bool dir[4] = { true, false, false, false };

	auto now = high_resolution_clock::now();

	if (_lastMove[id] + 1s > now) return;
	_lastMove[id] = now;

	std::uniform_real_distribution<float> pitchUid(-45.0f, 45.0f);
	std::uniform_real_distribution<float> yawUid(0.0f, 360.0f);
	
	client->RegisterSend(PacketFactory::CSMovePacket(id, dir, yawUid(dre), pitchUid(dre)));

	//std::cout << "[MoveScenario] OnTick : Session[" << id << "]\n";
}

void MoveScenario::OnPacket(Client* client, const std::vector<char>& packet)
{
}