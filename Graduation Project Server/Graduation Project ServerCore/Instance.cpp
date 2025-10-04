#include "pch.h"
#include "Instance.h"

Instance::Instance(int id, InstanceType type, IGameContext& gameCtx)
	: _id(id), _type(type), _gameCtx(gameCtx) 
{
	_objMng = std::make_unique<ObjectManager>();
	_gameLogic = std::make_unique<GameLogic>(this);
}

void Instance::Update(float deltaTime)
{
	_gameLogic->LogicUpdate(deltaTime);
	_gameLogic->NetworkUpdate();
}

void Instance::AddPlayer(Session* session)
{
	auto character = std::make_shared<GameObject>(session->GetId(), this);
	character->AddComponent<TransformComponent>(vec3{ 0, 0, 0 });
	character->AddComponent<MovementComponent>();
	character->AddComponent<ActionComponent>();
	character->AddComponent<InputComponent>();

	_objMng->AddObject(character);
	session->SetCharacter(character);
	{
		std::unique_lock lock{ _mutex };
		_sessions.insert(std::make_pair(session->GetId(), session));
	}
}

void Instance::RemovePlayer(int sessionId)
{
	_objMng->RemoveObject(sessionId);
	{
		std::unique_lock lock{ _mutex };
		_sessions.erase(sessionId);
	}
}

void Instance::BroadCast(const std::vector<char>& packet, int exceptId)
{
	std::vector<Session*> sessions;
	{
		std::shared_lock lock{ _mutex };
		for (const auto& [id, session] : _sessions) {
			if (session) {
				sessions.push_back(session);
			}
		}
	}

	for (const auto& session : sessions) {
		if (session->GetState() != SessionState::ST_INGAME) continue;
		if (session->GetId() == exceptId) continue;
		session->RegisterSend(packet);
	}
}

void Instance::AddObject(const std::shared_ptr<GameObject>& obj)
{
	_objMng->AddObject(obj);
}

void Instance::RemoveObject(int id)
{
	_objMng->RemoveObject(id);
}

std::vector<std::shared_ptr<GameObject>> Instance::GetGameObjectList() const
{
	return _objMng->GetGameObjectList();
}
