#include "pch.h"
#include "SessionManager.h"

SessionManager::SessionManager(IGameContext& gameCtx) : _gameCtx(gameCtx), _nextSessionId(1)
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

	// TEMP : 나중에 Login Packet 만들면 OnSessionPacket에서 처리
	_gameCtx.GetGameWorld().GetInstance(0)->AddPlayer(session.get());

	// TEMP : Client에 자신의 id값 알려주기위해 임시 Login Packet Send
	session->RegisterSend(PacketFactory::SCLoginPacket(session->GetId()));
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

void SessionManager::SetCharacter(int sessionId, GameObject* character)
{
	Session* session = GetSession(sessionId);
	session->SetCharacter(character);
}

void SessionManager::OnSessionPacket(int sessionId, const std::vector<char>& packet)
{
	// 여기서 역직렬화? (나중에 Dispatcher로 따로 뺄 수도?)
	Protocol::GamePacket gamePacket;
	if (not gamePacket.ParseFromArray(packet.data(), packet.size())) {
		LOG_ERR("GamePaket parse failed");
		return;
	}

	const auto& header = gamePacket.header();
	switch (header.type()) {
	case Protocol::CS_INPUT: {
		Protocol::CS_INPUT_PACKET input;
		if (not input.ParseFromArray(gamePacket.body().data(), gamePacket.body().size())) {
			LOG_ERR("CS_INPUT_PACKET parse failed");
			return;
		}

		int instanceId = GetSession(sessionId)->GetCharacter()->GetInstanceId();
		auto instance = _gameCtx.GetGameWorld().GetInstance(instanceId);

		instance->GetInputSystem().HandleInput(sessionId, input);
		instance->GetGameLogic().OnPlayerAction(sessionId, input);
		break;
	}
	}
}