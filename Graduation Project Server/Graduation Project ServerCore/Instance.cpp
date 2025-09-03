#include "pch.h"
#include "Instance.h"

Instance::Instance(int id, InstanceType type, IGameContext& gameCtx)
	: _id(id), _type(type), _gameCtx(gameCtx) 
{
	_objMng = std::make_unique<ObjectManager>();
	_eventMng = std::make_unique<EventManager>();
	_gameLogic = std::make_unique<GameLogic>(this, _eventMng.get());
}

void Instance::Update(float deltaTime)
{
	_gameLogic->LogicUpdate(deltaTime);

	static float networkAcc{ 0.0f };
	networkAcc = networkAcc + deltaTime;
	
	if (networkAcc >= (1.0f / 60.0f)) {
		_gameLogic->NetworkUpdate();
		networkAcc = 0.0f;
	}
}

void Instance::AddPlayer(Session* session)
{
	auto character = std::make_shared<GameObject>(session->GetId(), this);
	character->AddComponent<TransformComponent>(vec3{ 0, 0, 0 });
	character->AddComponent<MovementComponent>();
	character->AddComponent<InputComponent>();

	_objMng->AddObject(character);
	session->SetCharacter(character.get());
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
