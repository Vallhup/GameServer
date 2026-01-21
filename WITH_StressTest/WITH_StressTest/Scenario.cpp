#include "pch.h"
#include "Scenario.h"

#include "Protocol.pb.h"
#include "PacketFactory.h"

/*---------------[ ConnectScenario ]---------------*/

void ConnectScenario::OnStart(Client* client)
{
	Protocol::CS_LOGIN_PACKET login;
	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_LOGIN_PACKET>
		(PacketType::CS_LOGIN, login);
	client->RegisterSend(data);
}

void ConnectScenario::OnTick(Client* client)
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
;
	std::uniform_real_distribution<float> yawUid(0.0f, 360.0f);
	std::uniform_int_distribution<int> xUid(-1, 1);
	std::uniform_int_distribution<int> zUid(-1, 1);
	
	Protocol::CS_MOVE_PACKET move;
	move.set_inputx(xUid(dre));
	move.set_inputz(zUid(dre));
	move.set_yaw(yawUid(dre));

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_MOVE_PACKET>(
		PacketType::CS_MOVE, move);

	client->RegisterSend(data);
}