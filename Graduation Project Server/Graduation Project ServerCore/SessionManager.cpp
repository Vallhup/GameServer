#include "pch.h"
#include "SessionManager.h"

SessionManager::SessionManager(IGameContext& gameCtx) : _gameCtx(gameCtx)
{
}

void SessionManager::AddSession(SOCKET clientSocket)
{
	LOG_DBG("Session[%d] Add", _nextSessionId.load());

	auto session = std::make_shared<Session>(_nextSessionId, clientSocket);
	session->SetPacketHandler([this](int sessionId, const std::vector<char>& packet)
		{
			OnSessionPacket(sessionId, packet);
		});
	session->RegisterRecv();

	_gameCtx.GetIocpCore().Register(session);
	{
		std::unique_lock lock{ _mutex };
		_sessions.insert(std::make_pair(_nextSessionId++, session));
	}
}

void SessionManager::RemoveSession(int sessionId)
{
	std::unique_lock lock{ _mutex };
	_sessions.erase(sessionId);
}

void SessionManager::OnSessionPacket(int sessionId, const std::vector<char>& packet)
{
	//_gameCtx.GetGameWorld().GetInstance(instanceId).GetCharacter(sessionId);
}