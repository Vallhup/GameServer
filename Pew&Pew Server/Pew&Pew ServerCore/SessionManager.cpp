#include "pch.h"
#include "SessionManager.h"

SessionManager::SessionManager(IGameContext& gameCtx) : _gameCtx(gameCtx)
{
	_sessionIds.resize(MAX_SESSION);
	std::iota(_sessionIds.begin(), _sessionIds.end(), 0);
}

void SessionManager::AcceptSession(SOCKET clientSocket)
{
	if (INVALID_SOCKET == clientSocket) {
		return;
	}

	int sessionId = GenerateSessionId();
	if (sessionId == -1) {
		closesocket(clientSocket);
		return;
	}

	u_long mode{ 1 };
	ioctlsocket(clientSocket, FIONBIO, &mode);

	auto session = std::make_shared<Session>(sessionId, clientSocket);

	session->SetPacketHandler([this](int sessionId, const std::vector<char>& packet)
		{
			OnSessionPacket(sessionId, packet);
		});
	session->OnConnect();
	AddSession(session);
}

void SessionManager::CloseSession(int sessionId)
{
	auto character = _gameCtx.GetCharacterManager().GetCharacter(sessionId);
	auto room = _gameCtx.GetRoomManager().GetRoomByCharacter(sessionId);
	
	if (character and room) {
		room->BroadCast(PacketFactory::SCRemovePacket(*character));
		room->RemoveCharacter(character->GetId());
	}

	RemoveSession(sessionId);
	_gameCtx.GetCharacterManager().RemoveCharacter(sessionId);
	
	_sessionIds.push_back(sessionId);
}

void SessionManager::AddSession(const std::shared_ptr<Session>& session)
{
	std::unique_lock lock{ _mutex };
	_sessions.insert(std::make_pair(session->GetId(), session));
}

void SessionManager::RemoveSession(int sessionId)
{
	std::unique_lock lock{ _mutex };
	_sessions.erase(sessionId);
}

std::shared_ptr<Session> SessionManager::GetSession(int sessionId) const
{
	std::shared_lock lock{ _mutex };
	auto it = _sessions.find(sessionId);
	return (it != _sessions.end()) ? it->second : nullptr;
}

std::vector<std::shared_ptr<Session>> SessionManager::GetSessionList() const
{
	std::shared_lock lock{ _mutex };

	std::vector<std::shared_ptr<Session>> sessionList;
	sessionList.reserve(_sessions.size());

	for (const auto& [id, session] : _sessions) {
		sessionList.push_back(session);
	}

	return sessionList;
}

int SessionManager::GenerateSessionId()
{
	if (_sessionIds.empty()) {
		return -1;
	}

	int id = _sessionIds.back();
	_sessionIds.pop_back();

	return id;
}

void SessionManager::OnSessionPacket(int sessionId, const std::vector<char>& packet)
{
	_gameCtx.GetGameLogic().OnPlayerAction(sessionId, packet);
}