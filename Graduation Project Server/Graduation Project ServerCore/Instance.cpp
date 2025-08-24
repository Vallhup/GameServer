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