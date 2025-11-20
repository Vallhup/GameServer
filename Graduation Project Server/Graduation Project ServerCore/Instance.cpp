#include "pch.h"
#include "Instance.h"

Instance::Instance(int id, InstanceType type, IGameContext& gameCtx)
	: _id(id), _type(type), _gameCtx(gameCtx) 
{
	//_objMng = std::make_unique<ObjectManager>();
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
		CollisionUpdate();
		_gameLogic->NetworkUpdate();

		_gameCtx.GetSessionManager().FlushAllSessions();

		// Update Flag 초기화
		_isUpdating.store(false);
	}
}

void Instance::EnqueueJob(const std::function<void()>& job)
{
	auto timerJob = new TimerJob(job, std::chrono::steady_clock::now());
	_jobQueue.Push(timerJob);
}

void Instance::AddPlayer(Session* session)
{
	auto character = std::make_unique<GameObject>(session->GetId(), this);
	character->AddComponent<TransformComponent>(vec3{ 0, 0, 0 });
	character->AddComponent<MovementComponent>();
	character->AddComponent<ActionComponent>();
	character->AddComponent<InputComponent>();

	auto collComp = character->AddComponent<CollisionComponent>();
	auto shapes = _gameCtx.GetCollisionManager().GetShapes("Knight");

	collComp->SetShapes(std::move(shapes));

	session->SetCharacter(character.get());
	session->SetState(SessionState::ST_INGAME);
	session->RegisterSend(PacketFactory::SCLoginPacket(session->GetId()));

	//_objMng->AddObject(std::move(character));
	_objects.insert(std::make_pair(character->GetId(), std::move(character)));
	_sessions.insert(session->GetId());

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
	//_objMng->RemoveObject(sessionId);
	_sessions.erase(sessionId);
}

void Instance::BroadCast(const std::vector<char>& packet, int exceptId)
{
	for (const auto& id : _sessions) {
		if (Session* session = _gameCtx.GetSessionManager().GetSession(id)) {
			if (session->GetState() != SessionState::ST_INGAME) continue;
			if (session->GetId() == exceptId) continue;

			session->RegisterSend(packet);
		}
	}
}

void Instance::AddObject(std::unique_ptr<GameObject> obj)
{
	//_objMng->AddObject(std::move(obj));
}

void Instance::RemoveObject(int id)
{
	//_objMng->RemoveObject(id);
}

GameObject* Instance::GetGameObject(int id) const
{
	auto it = _objects.find(id);
	if (it != _objects.end()) {
		return it->second.get();
	}

	return nullptr;
}

std::vector<GameObject*> Instance::GetGameObjectList() const
{
	std::vector<GameObject*> objectList;
	objectList.reserve(_objects.size());

	for (const auto& [id, object] : _objects) {
		objectList.push_back(object.get());
	}

	return objectList;
}

void Instance::DequeueJobs()
{
	Job* job{ nullptr };
	while (_jobQueue.TryPop(job)) {
		if (job) {
			job->Execute();
			delete job;
		}
	}
}

void Instance::CollisionUpdate()
{
	auto objects = GetGameObjectList();
	const size_t n = objects.size();

	for (size_t i = 0; i < n; i++) {
		auto* objA = objects[i];
		auto* colA = objA->GetComponent<CollisionComponent>();
		if (!colA) continue;

		auto* trA = objA->GetComponent<TransformComponent>();
		if (!trA) continue;

		for (size_t j = i + 1; j < n; j++) {
			auto* objB = objects[j];
			auto* colB = objB->GetComponent<CollisionComponent>();
			if (!colB) continue;

			auto* trB = objB->GetComponent<TransformComponent>();
			if (!trB) continue;

			// 충돌체 비교
			for (auto& sA : colA->GetShapes()) {
				if (!sA->IsActive()) continue;

				for (auto& sB : colB->GetShapes()) {
					if (!sB->IsActive()) continue;

					// ★ 1) 월드 기반 충돌체 생성
					auto worldA = sA->ToWorldShape(*trA);
					auto worldB = sB->ToWorldShape(*trB);

					worldA->Print("A");
					worldB->Print("B");

					// ★ 2) 충돌 검사
 					if (worldA->CheckCollision(*worldB)) {

						// ★ 3) 타입별 반응
						HandleCollision(objA, objB, *sA, *sB);
					}
				}
			}
		}
	}
}

void Instance::HandleCollision(GameObject* a, GameObject* b, const CollisionShape& sA, const CollisionShape& sB)
{
	//LOG_ERR("Object[%d] & Object[%d] Collision", a->GetId(), b->GetId());
}
