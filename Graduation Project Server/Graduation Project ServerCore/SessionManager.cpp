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
	case Protocol::PacketType::CS_LOGIN: {
		Protocol::CS_LOGIN_PACKET login;
		if (not login.ParseFromArray(gamePacket.body().data(), gamePacket.body().size())) {
			LOG_ERR("CS_LOGIN_PACKET parse failed");
			return;
		}

		// TEMP : Login Packet에 Character Data 포함? 
		//        아니면 접속 타이밍, 무결성만 검사하고 따로 Character Select Packet?
		auto session = GetSession(sessionId);

		// TEMP : 0번 Instance는 Server Town 고정
		//        나중에 Character 별로 Server Town 나뉘어지면 별도로 분기
		_gameCtx.GetGameWorld().GetInstance(0)->AddPlayer(session);
		session->RegisterSend(PacketFactory::SCLoginPacket(session->GetId()));

		const vec3 pos = session->GetCharacter()->GetComponent<TransformComponent>()->GetPosition();
		Protocol::Vec3 packetPos;
		packetPos.set_x(pos.x);
		packetPos.set_y(pos.y);
		packetPos.set_z(pos.z);

		session->RegisterSend(PacketFactory::SCAddPacket(sessionId, packetPos));

		break;
	}
	case Protocol::CS_INPUT: {
		Protocol::CS_INPUT_PACKET input;
		if (not input.ParseFromArray(gamePacket.body().data(), gamePacket.body().size())) {
			LOG_ERR("CS_INPUT_PACKET parse failed");
			return;
		}

		int instanceId = GetSession(sessionId)->GetCharacter()->GetInstanceId();
		auto instance = _gameCtx.GetGameWorld().GetInstance(instanceId);

		instance->GetGameLogic().OnPlayerAction(sessionId, input);
		break;
	}
	}
}