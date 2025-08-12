#include "pch.h"
#include "GameLogic.h"

GameLogic::GameLogic(IGameContext& gameCtx) : _gameCtx(gameCtx)
{
	RegisterHandlers();
}

void GameLogic::LogicUpdate(float deltaTime)
{
	// TODO : Update Object
}

void GameLogic::NetworkUpdate()
{
	// TODO : Logic Result Send
}

void GameLogic::OnPlayerAction(int sessionId, const std::vector<char>& packet)
{
	const unsigned char packetType = packet[1];

	auto it = _packetHandlers.find(packetType);
	if (it != _packetHandlers.end()) {
		it->second(sessionId, packet);
	}

	else {
		LOG_ERR("Unknown Packet Type : %d", packetType);
	}
}

void GameLogic::RegisterHandlers()
{
	// TODO : Register Handler Functions
}
