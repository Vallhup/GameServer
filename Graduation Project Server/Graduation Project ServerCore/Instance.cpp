#include "pch.h"
#include "Instance.h"

void Instance::AddSession(Session* session)
{
	std::unique_lock lock{ _mutex };
	_sessions.insert(std::make_pair(session->GetId(), session));
}

void Instance::RemoveSession(int sessionId)
{
	std::unique_lock lock{ _mutex };
	_sessions.erase(sessionId);
}

void Instance::BroadCast(const std::vector<char>& packet, int exceptId = -1)
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

void Instance::AddObject(const std::shared_ptr<DynamicGameObject>& obj)
{
	_objMng->AddDynamicObject(obj);
}

void Instance::RemoveObject(const ObjectId& id)
{
	_objMng->RemoveObject(id);
}

void TownInstance::Update(float deltaTime)
{
	// TODO : Object Update
}

void TownInstance::LoadStaticGameObject()
{
	// TEMP : 각 Instance에 맞는 Static Object Load

	auto temp = std::make_shared<StaticGameObject>();
	_objMng->AddStaticObject(temp);
}

void TownInstance::Stop()
{
	// TODO : Resource 정리, 상위 Class (GameWorld or IGameContext)에 삭제 알림
}