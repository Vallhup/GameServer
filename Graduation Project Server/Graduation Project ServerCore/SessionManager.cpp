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

	_gameCtx.GetIocpCore().Register(session);
	session->RegisterRecv();
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

Session* SessionManager::GetSession(int sessionId)
{
	std::shared_lock lock{ _mutex };
	auto it = _sessions.find(sessionId);
	if (it != _sessions.end()) {
		return it->second.get();
	}

	return nullptr;
}

void SessionManager::OnSessionPacket(int sessionId, const std::vector<char>& packet)
{
	// 여기서 역직렬화? (나중에 Dispatcher로 따로 뺄 수도?)
	Protocol::PacketHeader header;
	header.ParseFromArray(packet.data(), header.ByteSizeLong());

	switch (header.type()) {
	case Protocol::CS_INPUT: {
		Protocol::CS_INPUT_PACKET input;
		input.ParseFromArray(packet.data(), packet.size());

		// TODO : session과 연결된 Character의 Instance 찾아서 
		// 그 Instance의 InputSystem HandleInput + GameLogic OnPlayerAction 호출
		int instanceId = GetSession(sessionId)->GetCharacter()->GetInstanceId();
		auto instance = _gameCtx.GetGameWorld().GetInstance(instanceId);

		instance->GetInputSystem().HandleInput(input);
		instance->GetGameLogic().OnPlayerAction(sessionId, input);
	}
	}
}