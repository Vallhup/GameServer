#include "pch.h"
#include "Service.h"
#include "Listener.h"

Service::Service(std::shared_ptr<IocpCore> core, int maxSessionCount)
	: _iocpCore(core), _maxSessionCount(maxSessionCount), _chatManager(*this)
{
	_sessionCount = 0;
	_npcCount = maxSessionCount;
}

bool Service::Start()
{
	LOG_INF("Start Service");

	// 0. running Flag 설정
	_running.store(true);

	// 1. WinSock 초기화
	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
		LOG_ERR("WSAStartup Error");
		return false;
	}

	// 2. NPC Initialize / NPC Timer Thread Start
	InitNpcs(MAX_NPC);
	StartNpcTimerThread();

	// 3. Listener에서 Accept 시작
	_listener = std::make_shared<Listener>();
	if (_listener == nullptr) {
		LOG_ERR("Listener allocation failed");
		return false;
	}

	std::shared_ptr<Service> service = shared_from_this();
	_iocpCore->SetService(service);
	if (_listener->StartAccept(service) == false) {
		LOG_ERR("Listener StartAccept filed");
		return false;
	}

	// 4. Worker Thread Start
	unsigned int threadCount = std::thread::hardware_concurrency();
	_workers.reserve(threadCount);
	for (unsigned int i = 0; i < threadCount; ++i) {
		_workers.emplace_back([this]()
			{
				while (_running.load()) {
					if (not _iocpCore->Dispatch()) {
						int error = WSAGetLastError();

						if (_running.load()) {
							if (error == WSAECONNRESET || error == WSAENOTCONN || error == ERROR_NETNAME_DELETED || error == WSA_OPERATION_ABORTED) {
								LOG_WRN("IOCP Dispatch expected error: %d", error);
							}
							else {
								LOG_ERR("IOCP Dispatch critical error: %d", error);
							}
						}

						break;
					}
				}
			});
	}

	return true;
}

void Service::CloseService()
{
	// 0) _running Flag 설정
	_running.store(false);

	// 1) Accept 종료
	_listener->StopAccept();

	// 2) Worker Thread join
	for (size_t i = 0; i < _workers.size(); ++i) {
		PostQueuedCompletionStatus(_iocpCore->GetHandle(), 0, 0, nullptr);
	}

	for (std::thread& worker : _workers) {
		if (worker.joinable()) {
			worker.join();
		}
	}
	_workers.clear();

	// 3) Timer Thread join
	if (_npcTimerThread.joinable()) {
		_npcTimerThread.join();
	}

	// 4) Session 정리
	for (auto& [id, session] : _objects) {
		if (id >= MAX_USER) continue;
		GameSessionPtr p = static_pointer_cast<GameSession>(session.load());
		if (nullptr != p) {
			CancelIoEx(p->GetHandle(), nullptr);
		}
	}
	_objects.clear();

	// 5) Listener 해제
	_listener.reset();

	// 6) IOCP Handle 해제
	_iocpCore.reset();

	// 7) WinSock 정리
	WSACleanup();
}

std::shared_ptr<GameObject> Service::FindObject(int id)
{
	auto it = _objects.find(id);
	if (it == _objects.end()) {
		return nullptr;
	}

	auto object = it->second.load();
	return object;
}

void Service::AddObject(const std::shared_ptr<GameObject> object)
{
	switch (object->GetType()) {
	case ObjectType::Player: object->SetId(_sessionCount++); break;
	case ObjectType::Npc:	 object->SetId(_npcCount++); break;
	}

	_objects.insert(std::make_pair(object->GetId(), object));
}

void Service::ReleaseObject(const std::shared_ptr<GameObject> object)
{
	int objectId = object->GetId();

	switch (object->GetType()) {
	case ObjectType::Player: {
		auto session = static_pointer_cast<GameSession>(object);
		FinalizeRelease(session);

		break;
	}
	case ObjectType::Npc: {
		auto npc = static_pointer_cast<NPC>(object);

		// 1. Sector에서 npc 제거
		leaveSector(npc);

		// 2. Container에서 제거 / npcCount - 1
		auto it = _objects.find(npc->GetId());
		if (it == _objects.end()) {
			return;
		}

		it->second = nullptr;
		--_npcCount;
		
		break;
	}
	}

	LOG_INF("Object %d released (type %d)", objectId, static_cast<int>(object->GetType()));
}

void Service::FinalizeRelease(const std::shared_ptr<GameSession>& session)
{
	int id = session->GetId();

	auto it = _objects.find(id);
	if (it == _objects.end()) {
		return;
	}

	auto s = it->second.load();
	if (nullptr == s) {
		return;
	}

	// 1. viewList 동기화
	std::unordered_set<int> viewList;
	{
		std::shared_lock vl{ session->_viewLock };
		viewList = session->_viewList;
	}

	for (int objId : viewList) {
		if (objId >= MAX_USER) continue;

		auto target = static_pointer_cast<GameSession>(FindObject(objId));
		if (nullptr == target) continue;

		auto packetForTarget = PacketFactory::BuildRemovePacket(*session);
		target->Send(packetForTarget);
	}

	// 2. Sector에서 session 제거
	leaveSector(session);

	// 3. Container에서 제거 / sessionCount - 1
	it->second = nullptr;
	--_sessionCount;

	LOG_INF("Session %d finalized and erased", id);
}

void Service::InitNpcs(int npcCount)
{
	for (int i = 0; i < npcCount; ++i) {
		std::string name = "NPC" + std::to_string(i);
		auto npc = std::make_shared<NPC>(MAX_USER + i, rand() % W_WIDTH, rand() % W_HEIGHT, name);
		npc->SetService(shared_from_this());
		AddObject(npc);
		enterSector(npc);
	}
}

void Service::StartNpcTimerThread()
{
	using namespace std::chrono;

	_npcTimerThread = std::thread([this]()
		{
			while (_running.load()) {
				Event event;
				while (_timerQueue.try_pop(event)) {
					auto now = high_resolution_clock::now();
					if (event.wakeupTime > now) {
						_timerQueue.push(event);
						break;
					}

					NpcOver* npcOver = new NpcOver(event.objId);
					PostQueuedCompletionStatus(_iocpCore->GetHandle(), 0, event.objId, reinterpret_cast<LPOVERLAPPED>(npcOver));
				}
				std::this_thread::sleep_for(1ms);
			}
		});
}

void Service::OnPlayerLogin(const std::shared_ptr<GameSession>& session)
{
	// 1. Player 위치 기준 시야 범위 내 Sector 범위 계산
	auto [xRange, yRange] = Sector::getSectorRange(session->GetX(), session->GetY());

	// 2. 시야에 걸치는 모든 Sector 순회
	for (int sx = xRange.first; sx <= xRange.second; ++sx) {
		for (int sy = yRange.first; sy <= yRange.second; ++sy) {
			// 3. 해당 Sector의 Object List Get
			std::unordered_set<int> sectorObjects;
			_sectors[sx][sy].collectObject(sectorObjects);

			for (int object : sectorObjects) {
				if (object < MAX_USER) continue;
				if (auto npc = static_pointer_cast<NPC>(FindObject(object))) {
					npc->WakeUp(true);
				}

				else {
					LOG_ERR("OnPlayerLogin FindObject failed");
				}
			}
		}
	}
}

void Service::OnPlayerMove(const std::shared_ptr<GameSession>& session)
{
	// 1. Player 위치 기준 시야 범위 내 Sector 범위 계산
	auto [xRange, yRange] = Sector::getSectorRange(session->GetX(), session->GetY());

	// 2. 시야에 걸치는 모든 Sector 순회
	for (int sx = xRange.first; sx <= xRange.second; ++sx) {
		for (int sy = yRange.first; sy <= yRange.second; ++sy) {
			// 3. 해당 Sector의 Object List Get
			std::unordered_set<int> sectorObjects;
			_sectors[sx][sy].collectObject(sectorObjects);

			for (int object : sectorObjects) {
				if (object < MAX_USER) continue;
				if (auto npc = static_pointer_cast<NPC>(FindObject(object))) {
					npc->WakeUp();
				}

				else {
					LOG_ERR("OnPlayerMove FindObject failed");
				}
			}
		}
	}
}

void Service::enterSector(const std::shared_ptr<GameObject>& object)
{
	auto [sx, sy] = Sector::getSector(object->_x, object->_y);
	_sectors[sx][sy].addObject(object->_id);
}

void Service::leaveSector(const std::shared_ptr<GameObject>& object)
{
	auto [sx, sy] = Sector::getSector(object->_x, object->_y);
	_sectors[sx][sy].removeObject(object->_id);
}

void Service::enterSector(const std::shared_ptr<GameObject>& object, int sx, int sy)
{
	_sectors[sx][sy].addObject(object->_id);
}

void Service::leaveSector(const std::shared_ptr<GameObject>& object, int sx, int sy)
{
	_sectors[sx][sy].removeObject(object->_id);
}

std::unordered_set<int> Service::collectVisibleObjects(const std::shared_ptr<GameObject>& object) const
{
	auto [xRange, yRange] = Sector::getSectorRange(object->_x, object->_y);
	std::unordered_set<int> result;

	for (int sx = xRange.first; sx <= xRange.second; ++sx) {
		for (int sy = yRange.first; sy <= yRange.second; ++sy) {
			_sectors[sx][sy].collectObject(result);
		}
	}

	return result;
}

void Service::OnChatRequest(int senderId, const char* msg)
{
	_chatManager.HandleMessage(senderId, msg);
}

void Service::Broadcast(const std::vector<char>& buf)
{
	for (auto& [id, object] : _objects) {
		if (id >= MAX_USER) continue;
		if (GameSessionPtr p = static_pointer_cast<GameSession>(object.load())) {
			p->Send(buf);
		}
	}
}

std::shared_ptr<Service> Service::Create(std::shared_ptr<IocpCore> core, int maxSessionCount)
{
	std::shared_ptr<Service> service = std::make_shared<Service>(core, maxSessionCount);
	return service;
}