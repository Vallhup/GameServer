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
	bool expected{ false };
	if(_isUpdating.compare_exchange_strong(expected, true)) {
		// 내부 Job Queue 처리
		DequeueJobs();

		// 게임 로직 업데이트
		_gameLogic->LogicUpdate(deltaTime);
		_gameLogic->NetworkUpdate();

		// Update Flag 초기화
		_isUpdating.store(false);
	}
}

void Instance::EnqueueJob(const std::function<void()>& job)
{
	auto timerJob = std::make_shared<TimerJob>(job, std::chrono::steady_clock::now());
	_jobQueue.Push(timerJob);
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
	session->SetState(SessionState::ST_INGAME);
	session->RegisterSend(PacketFactory::SCLoginPacket(session->GetId()));

	Protocol::Vec3 packetPos;
	if (auto character = session->GetCharacter()) {
		if (auto trComp = character->GetComponent<TransformComponent>()) {
			const vec3 pos = trComp->GetPosition();

			packetPos.set_x(pos.x);
			packetPos.set_y(pos.y);
			packetPos.set_z(pos.z);
		}
	}

	BroadCast(PacketFactory::SCAddPacket(session->GetId(), packetPos));

	for (const auto& sess : _gameCtx.GetSessionManager().GetSessionList()) {
		if (sess->GetId() == session->GetId()) continue;
		if (auto character = sess->GetCharacter()) {
			if (auto trComp = character->GetComponent<TransformComponent>()) {
				const vec3 sessPos = trComp->GetPosition();
				packetPos.set_x(sessPos.x);
				packetPos.set_y(sessPos.y);
				packetPos.set_z(sessPos.z);

				session->RegisterSend(PacketFactory::SCAddPacket(sess->GetId(), packetPos));
			}
		}
	}
}

void Instance::RemovePlayer(int sessionId)
{
	_objMng->RemoveObject(sessionId);
	{
		//std::unique_lock lock{ _mutex };
		_sessions.erase(sessionId);
	}
}

void Instance::BroadCast(const std::vector<char>& packet, int exceptId)
{
	//std::vector<Session*> sessions;
	{
		//std::shared_lock lock{ _mutex };
		/*for (const auto& [id, session] : _sessions) {
			if (session) {
				sessions.push_back(session);
			}
		}*/
	}

	for (const auto& [id, session] : _sessions) {
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

void Instance::DequeueJobs()
{
	std::shared_ptr<Job> job;
	while (_jobQueue.TryPop(job)) {
		if (job) {
			job->Execute();
		}
	}
}
