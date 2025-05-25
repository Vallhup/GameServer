#include "pch.h"
#include "Service.h"
#include "Listener.h"
#include "Party.h"

Service::Service(std::shared_ptr<IocpCore> core, int maxSessionCount)
	: _iocpCore(core), _maxSessionCount(maxSessionCount)
{
	_nextPlayerId = 0;
	_nextNpcId = MAX_USER;
	_nextPartyId = 0;

	for (auto& row : _navigationMap) {
		row.fill(true);
	}
}

bool Service::Start()
{
	LOG_INF("Start Service");

	// temp : rand() Seed Setting
	srand(static_cast<unsigned int>(time(nullptr)));

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

int Service::AllocateObjectId(const std::shared_ptr<GameObject>& object)
{
	int id;

	switch (object->GetType()) {
	case ObjectType::Player: {
		if (not _freePlayerIds.empty()) {
			if (_freePlayerIds.try_pop(id)) {
				return id;
			}
		}

		if (_nextPlayerId < MAX_USER) {
			return _nextPlayerId++;
		}

		break;
	}
	case ObjectType::Npc: {
		if (not _freeNpcIds.empty()) {
			if (_freeNpcIds.try_pop(id)) {
				return id;
			}
		}

		if (_nextNpcId < MAX_USER + MAX_NPC) {
			return _nextNpcId++;
		}

		break;
	}
	}

	return -1;
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

int Service::AddObject(const std::shared_ptr<GameObject> object)
{
	int id = AllocateObjectId(object);
	if (id == -1) {
		LOG_WRN("Too Many IDs");
		return -1;
	}

	object->SetId(id);
	{
		std::lock_guard<std::mutex> lock{ _idMutex };
		_objects.insert(std::make_pair(id, object));
	}

	return id;
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
		int id = npc->GetId();

		// 1. Sector에서 npc 제거
		leaveSector(npc);

		// 2. Container에서 제거 / npcCount - 1
		auto it = _objects.find(id);
		if (it == _objects.end()) {
			return;
		}

		{
			std::lock_guard<std::mutex> lock{ _idMutex };
			_objects.unsafe_erase(id);
			_freeNpcIds.push(id);
		}
		
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
	//std::unordered_set<int> viewList;
	/*{
		std::shared_lock vl{ session->_viewLock };
		viewList = session->_viewList;
	}*/
	std::unordered_set<int> viewList = *(session->_viewList.load());

	for (int objId : viewList) {
		if (objId >= MAX_USER) continue;

		auto target = static_pointer_cast<GameSession>(FindObject(objId));
		if (nullptr == target) continue;

		auto packetForTarget = PacketFactory::BuildRemovePacket(*session);
		target->Send(packetForTarget);
	}

	// 2. Sector에서 session 제거
	leaveSector(session);

	// 3. 파티있으면 파티 상태 Update
	if (auto party = session->_party.lock()) {
		party->Update();
	}

	// 4. Container에서 제거
	{
		std::lock_guard<std::mutex> lock{ _idMutex };
		_objects.unsafe_erase(id);
		_freePlayerIds.push(id);
	}

	LOG_INF("Session %d finalized and erased", id);
}

void Service::InitNpcs(int npcCount)
{
	for (int i = 0; i < npcCount; ++i) {
		std::string name = "NPC" + std::to_string(i);
		auto npc = std::make_shared<NPC>(-1, rand() % W_WIDTH, rand() % W_HEIGHT, name);
		npc->SetService(shared_from_this());
		AddObject(npc);
		enterSector(npc);
		_iocpCore->Register(npc);
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

					OperationType opType;
					switch (event.eventId) {
					case EV_MOVE:			opType = NpcMove; break;
					case EV_HEAL:			opType = NpcHeal; break;
					case EV_ATTACK:			opType = NpcAttack; break;
					case EV_PLAYER_HEAL:	opType = Heal; break;
					default: continue;
					}

					EventOver* eventOver = new EventOver(opType, event.objId);

					if (event.objId < MAX_USER) {
						if (auto player = static_pointer_cast<GameSession>(FindObject(event.objId))) {
							eventOver->_owner = player;
						}
					}
					
					else {
						if (auto npc = static_pointer_cast<NPC>(FindObject(event.objId))) {
							eventOver->_owner = npc;
						}
					}
			
					PostQueuedCompletionStatus(_iocpCore->GetHandle(), 0, event.objId, reinterpret_cast<LPOVERLAPPED>(eventOver));
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

bool Service::OnPacket(const std::shared_ptr<GameSession>& session, const std::vector<char>& packet)
{
	char packetType = packet[1];

	switch (packetType) {
	case CS_LOGIN:			return OnLogin(session, packet);
	case CS_LOGOUT:			return OnLogout(session, packet);
	case CS_MOVE:			return OnMove(session, packet);
	case CS_ATTACK:			return OnAttack(session, packet);
	case CS_CHAT:			return OnChat(session, packet);
	case CS_PARTY_REQUEST:	return OnPartyRequest(session, packet);
	case CS_PARTY_RESPONSE: return OnPartyResponse(session, packet);
	case CS_PARTY_LEAVE:	return OnPartyLeave(session);
	case CS_USE_ITEM:		return OnUseItem(session, packet);

	default:
		LOG_WRN("Packet Type Error");
		return false;
	}
}

bool Service::OnLogin(const std::shared_ptr<GameSession>& session, const std::vector<char>& packet)
{
	// 0. ALLOC 상태를 INGAME으로 변경
	State expect = ST_ALLOC;
	if (not session->_state.compare_exchange_strong(expect, ST_INGAME)) {
		LOG_WRN("Session state is not Alloc");
		return false;
	}

	// 1. packet 파싱
	auto requestPacket = PacketFactory::Deserialize<CS_LOGIN_PACKET>(packet);
	requestPacket.name[NAME_SIZE - 1] = '\0';

	// 2. Session name 설정
	session->_name = requestPacket.name;

	// 3. Session Container에 등록
	AddObject(session);

	// 4. Login / Stat Packet Send
	auto loginPacket = PacketFactory::BuildLoginPacket(*session);
	auto statPacket = PacketFactory::BuildStatChangePacket(*session);

	session->Send(loginPacket);
	session->Send(statPacket);

	// 5. Sector에 등록
	enterSector(session);

	// 6. OnPlayerLogin 호출
	OnPlayerLogin(session);

	// 7. viewList 동기화
	auto newViewList = ViewListHelper::collectViewList(session, shared_from_this());
	auto oldViewList = ViewListHelper::updateViewList(session->_viewList, newViewList);

	ViewListDiff diffViewList = ViewListHelper::calcViewListDiff(oldViewList, newViewList);

	for (int addPlayer : diffViewList.addViewList) {
		auto object = FindObject(addPlayer);
		if (nullptr == object) {
			continue;
		}

		auto packetForSelf = PacketFactory::BuildAddPacket(*object);
		session->Send(packetForSelf);

		if (object->GetType() == ObjectType::Player) {
			auto target = static_pointer_cast<GameSession>(object);
			auto packetForTarget = PacketFactory::BuildAddPacket(*session);
			target->Send(packetForTarget);
		}
	}

	// 8. 자동 회복 Event Push
	_timerQueue.push(Event{ session->GetId(),
		std::chrono::high_resolution_clock::now() + std::chrono::seconds(5),
		EV_PLAYER_HEAL, 0 });

	LOG_DBG("Process Login Packet Success");
	return true;
}

bool Service::OnLogout(const std::shared_ptr<GameSession>& session, const std::vector<char>& packet)
{
	session->Close();
	return true;
}

bool Service::OnMove(const std::shared_ptr<GameSession>& session, const std::vector<char>& packet)
{
	// 0. INGAME이 아니면 실행 X
	if (session->_state.load() != ST_INGAME) {
		LOG_WRN("Session state is not Ingame");
		return false;
	}

	// 1. packet 파싱
	auto requestPacket = PacketFactory::Deserialize<CS_MOVE_PACKET>(packet);
	session->_lastMoveTime = requestPacket.move_time;

	//// 2. lastMoveTime과 현재 시각 계산해서 1초에 1번씩 움직이도록 제한
	/*auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	if ((now - session->_lastMoveTime) < 1000) {
		LOG_INF("Move Cooldown");
		return false;
	}
	session->_lastMoveTime = now;*/

	// 2. Pos 업데이트
	short x = session->_x;
	short y = session->_y;

	auto oldSector = Sector::getSector(x, y);

	switch (requestPacket.direction) {
	case UP:	if (y > 0)				y--; break;
	case DOWN:	if (y < W_HEIGHT - 1)	y++; break;
	case LEFT:	if (x > 0)				x--; break;
	case RIGHT: if (x < W_WIDTH - 1)	x++; break;
	}

	auto newSector = Sector::getSector(x, y);

	session->_x = x;
	session->_y = y;

	// 3. Sector 동기화
	if (oldSector != newSector) {
		leaveSector(session, oldSector.first, oldSector.second);
		enterSector(session, newSector.first, newSector.second);
	}

	// 4. viewList 동기화
	auto newViewList = ViewListHelper::collectViewList(session, shared_from_this());
	auto oldViewList = ViewListHelper::updateViewList(session->_viewList, newViewList);

	ViewListDiff diffViewList = ViewListHelper::calcViewListDiff(oldViewList, newViewList);

	// 5. OnPlayerMove 호출
	OnPlayerMove(session);

	// 6. 각 Player에게 Packet Send
	auto myPacket = PacketFactory::BuildMovePacket(*session);
	session->Send(myPacket);

	for (int addPlayer : diffViewList.addViewList) {
		auto object = FindObject(addPlayer);
		if (nullptr == object) {
			continue;
		}

		auto packetForSelf = PacketFactory::BuildAddPacket(*object);
		session->Send(packetForSelf);

		if (object->GetType() == ObjectType::Player) {
			auto target = static_pointer_cast<GameSession>(object);
			auto packetForTarget = PacketFactory::BuildAddPacket(*session);
			target->Send(packetForTarget);
		}
	}

	for (int movePlayer : diffViewList.moveViewList) {
		auto object = FindObject(movePlayer);
		if (nullptr == object) {
			continue;
		}

		auto packetForSelf = PacketFactory::BuildMovePacket(*object);
		session->Send(packetForSelf);

		if (object->GetType() == ObjectType::Player) {
			auto target = static_pointer_cast<GameSession>(object);
			auto packetForTarget = PacketFactory::BuildMovePacket(*session);
			target->Send(packetForTarget);
		}
	}

	for (int removePlayer : diffViewList.removeViewList) {
		auto object = FindObject(removePlayer);
		if (nullptr == object) {
			continue;
		}

		auto packetForSelf = PacketFactory::BuildRemovePacket(*object);
		session->Send(packetForSelf);

		if (object->GetType() == ObjectType::Player) {
			auto target = static_pointer_cast<GameSession>(object);
			auto packetForTarget = PacketFactory::BuildRemovePacket(*session);
			target->Send(packetForTarget);
		}
	}

	LOG_DBG("Process Move Packet Success");
	return true;
}

bool Service::OnAttack(const std::shared_ptr<GameSession>& session, const std::vector<char>& packet)
{
	// 0. INGAME이 아니면 실행 X
	if (session->_state.load() != ST_INGAME) {
		LOG_WRN("Session state is not Ingame");
		return false;
	}

	// 1. packet 파싱
	auto requestPacket = PacketFactory::Deserialize<CS_ATTACK_PACKET>(packet);
	auto now = requestPacket.attack_time;

	// 2. 1초에 1번씩 공격
	//  - 지금은 client 시간 기준으로 맞추고 있음
	//    보안 등 생각하면 나중에는 서버 시간 기준으로 맞추고 client 시간은 보조로
	if (now < session->_lastAttackTime + 1000) {
		return false;
	}
	session->_lastAttackTime = now;

	// 3. 4방향 검사해서 Target Setting
	std::vector<std::shared_ptr<NPC>> targets;

	const std::array<std::pair<int, int>, 4> directions{ {
		{ 0, -1 },
		{ 0,  1 },
		{ -1, 0 },
		{ 1,  0 }
	} };

	auto visible = collectVisibleObjects(session);
	for (const auto& [dx, dy] : directions) {
		short nx = session->GetX() + dx;
		short ny = session->GetY() + dy;

		for (int id : visible) {
			if (id < MAX_USER) continue;

			auto npc = static_pointer_cast<NPC>(FindObject(id));
			if (nullptr == npc) continue;
			if ((nx == npc->GetX()) and (ny == npc->GetY()) and (npc->IsAlive())) {
				auto snapShot = session->_viewList.load();
				if (snapShot->contains(npc->GetId())) {
					targets.push_back(npc);
				}
			}
		}
	}

	// 4. 전투 로직 (Damage 계산, Exp 계산 등)
	int totalExp{ 0 };
	for (auto& npc : targets) {
		if (not npc->IsAlive()) continue;

		int damage = session->_damage;
		npc->_hp -= damage;

		// TODO : 타격 패킷 Send

		if (npc->_hp <= 0) {
			npc->_isAlive.store(false);
			npc->_hp = 0;

			int exp = npc->_level * npc->_level * 2;
			totalExp += exp;

			// TODO : Kill 패킷 Send

			// Temp : Monster Kill하면 Kill한 Player에게 30% 확률로 Hp Potion 1개 추가
			if (rand() % 100 < 30) {
				session->GetInventory()->AddItem(HpPotion, 1);
				session->Send(PacketFactory::BuildAddItemPacket(HpPotion, 1));
			}

			if(not npc->_healPending.exchange(true)) {
				_timerQueue.push(Event{ npc->_id,
					std::chrono::high_resolution_clock::now() + std::chrono::seconds(30),
					EV_HEAL, 0 });
			}

			// 시야에서 제거
			auto visiblePlayers = collectVisibleObjects(npc);
			for (int id : visiblePlayers) {
				if (id >= MAX_USER) continue;

				auto player = static_pointer_cast<GameSession>(FindObject(id));
				if (nullptr == player) continue;

				// ViewList 동기화
				auto newViewList = ViewListHelper::collectViewList(player, shared_from_this());

				auto oldSnapShot = player->_viewList.load();
				auto oldViewList = ViewListHelper::updateViewList(player->_viewList, newViewList);
				
				if ((not npc->_isAlive.load()) and (oldSnapShot->contains(npc->GetId()))) {
					auto removePacket = PacketFactory::BuildRemovePacket(*npc);
					player->Send(removePacket);
				}
			}
		}
	}

	// 5. Exp 분배
	if (totalExp > 0) {
		if (auto party = session->_party.lock()) {
			party->ShareExp(totalExp);
		}

		else {
			session->_exp += totalExp;

			auto statPacket = PacketFactory::BuildStatChangePacket(*session);
			session->Send(statPacket);
		}
	}

	// 6. 공격 범위 알림 보내기
	SC_ATTACK_NOTIFY_PACKET p;
	p.size = sizeof(p);
	p.type = SC_ATTACK_NOTIFY;
	p.attackerId = session->GetId();
	p.x[0] = session->GetX() - 1; p.y[0] = session->GetY();
	p.x[1] = session->GetX() + 1; p.y[1] = session->GetY();
	p.x[2] = session->GetX();     p.y[2] = session->GetY() - 1;
	p.x[3] = session->GetX();     p.y[3] = session->GetY() + 1;

	for (int id : visible) {
		if (id >= MAX_USER) continue;
		if (auto target = std::static_pointer_cast<GameSession>(FindObject(id))) {
			target->Send(PacketFactory::Serialize(p));
		}
	}

	return true;
}

bool Service::OnChat(const std::shared_ptr<GameSession>& session, const std::vector<char>& packet)
{
	// 0. INGAME이 아니면 실행 X
	if (session->_state.load() != ST_INGAME) {
		LOG_WRN("Session state is not Ingame");
		return false;
	}

	// 1. Packet 파싱 / null termination
	auto requestPacket = PacketFactory::Deserialize<CS_CHAT_PACKET>(packet);
	requestPacket.message[CHAT_SIZE - 1] = '\0';

	// 2. OnChatRequest 호출
	OnChatRequest(session->GetId(), requestPacket.message);

	LOG_DBG("Process Chat Packet Success");
	return true;
}

bool Service::OnPartyRequest(const std::shared_ptr<GameSession>& session, const std::vector<char>& packet)
{
	// 0. INGAME이 아니면 실행 X
	if (session->_state.load() != ST_INGAME) {
		LOG_WRN("Session state is not Ingame");
		return false;
	}

	// 1. Packet 파싱
	auto requestPacket = PacketFactory::Deserialize<CS_PARTY_REQUEST_PACKET>(packet);

	// 2. Target Session Find
	auto it = _objects.find(requestPacket.targetId);
	if (it == _objects.end()) {
		LOG_WRN("Party Request Target Session Error %d", requestPacket.targetId);
		return false;
	}

	auto target = static_pointer_cast<GameSession>(it->second.load());
	if ((nullptr == target) or (target->_state.load() != ST_INGAME)) {
		LOG_WRN("Party Request Target Session Error %d", requestPacket.targetId);
		return false;
	}

	// 3. 이미 파티가 있는지 확인
	if (auto existingParty = target->_party.lock()) {
		LOG_INF("Player %d already in party %d", target->GetId(), existingParty->GetId());

		// 이미 다른 파티가 있으면 실패
		auto failPacket = PacketFactory::BuildPartyResultPacket(target->GetId(), false);
		session->Send(failPacket);

		return false;
	}

	// 4. 이미 다른 초대가 pending 중인지 확인
	if (auto exist = target->_pendingPartyRequester.load().lock()) {
		LOG_INF("Player %d already has a pending invite from %d", target->GetId(), exist->GetId());

		// 이미 다른 초대가 있으면 실패
		auto failPacket = PacketFactory::BuildPartyResultPacket(target->GetId(), false);
		session->Send(failPacket);

		return false;
	}

	// 5. Target Pending Party Requester Setting
	target->_pendingPartyRequester.store(session);

	// 6. Target Session에게 초대 알림 Send
	auto packetForTarget = PacketFactory::BuildPartyRequestPacket(session->GetId());
	target->Send(packetForTarget);

	return true;
}

bool Service::OnPartyResponse(const std::shared_ptr<GameSession>& session, const std::vector<char>& packet)
{
	// 0. INGAME이 아니면 실행 X
	if (session->_state.load() != ST_INGAME) {
		LOG_WRN("Session state is not Ingame");
		return false;
	}

	// 1. Packet 파싱
	auto responsePacket = PacketFactory::Deserialize<CS_PARTY_RESPONSE_PACKET>(packet);

	// 2. Pending Party Requester 파싱
	auto requester = session->ConsumePendingPartyRequester();
	if (nullptr == requester) {
		LOG_WRN("Party Requester is Null");
		return false;
	}

	// 3. 이미 파티에 속해 있으면 거절
	if (nullptr != session->_party.lock()) {
		LOG_INF("Player %d tried to accept invite but is already in party", session->GetId());

		requester->Send(PacketFactory::BuildPartyResultPacket(session->GetId(), false));
		return true;
	}

	// 4. requester가 속해 있는 파티가 정원이 찼으면 자동 거절
	if (auto existing = requester->_party.lock()) {
		if (existing->GetMemberInfos().size() >= Party::MAX_MEMBER) {
			LOG_INF("Party[%d] is full, cannot add player %d", existing->GetId(), session->GetId());
			
			requester->Send(PacketFactory::BuildPartyResultPacket(session->GetId(), false));
			return true;
		}
	}

	// 5. 실제로 수락 or 거절 하였을 경우 Send
	auto packetForTarget = PacketFactory::BuildPartyResultPacket(session->GetId(), responsePacket.acceptFlag);
	requester->Send(packetForTarget);

	// 거절했으면
	if (not responsePacket.acceptFlag) {
		// 파티 생성 X
		return true;
	}

	// 6. 파티 생성 / 합류 (기존 파티 없으면 생성 / 있으면 합류)
	std::shared_ptr<Party> party;
	// 기존 파티가 있으면 
	if (auto exist = requester->_party.lock()) {
		party = exist;

		// 기존 파티에 session만 추가
		party->AddMember(session);
	}

	// 기존 파티가 없으면
	else {
		int newId = _nextPartyId++;
		// 새로운 파티 만들어서
		party = std::make_shared<Party>(newId);
		{
			std::unique_lock lock{ _partyMutex };
			_parties[newId] = party;
		}

		// requester, session 모두 추가
		party->AddMember(requester);
		party->AddMember(session);
	}

	// 7. 파티 상태 Update
	party->Update();

	return true;
}

bool Service::OnPartyLeave(const std::shared_ptr<GameSession>& session)
{
	auto party = session->_party.lock();
	if (nullptr == party) {
		LOG_WRN("Leave Party is Null");
		return false;
	}

	// 1. 파티에서 제거
	party->RemoveMember(session->GetId());
	session->_party.reset();

	// 2. 파티 해산 or 파티 갱신
	if (party->Empty()) {
		party->Disband();
		OnPartyDisband(party->GetId());
	}

	else {
		party->Update();
	}

	return true;
}

void Service::OnPartyDisband(int partyId)
{
	std::shared_ptr<Party> party;
	{
		std::unique_lock lock{ _partyMutex };

		auto it = _parties.find(partyId);
		if (it == _parties.end()) {
			return;
		}

		party = it->second;
		_parties.erase(it);
	}
}

bool Service::OnUseItem(const std::shared_ptr<GameSession>& session, const std::vector<char>& packet)
{
	// 0. INGAME이 아니면 실행 X
	if (session->_state.load() != ST_INGAME) {
		LOG_WRN("Session state is not Ingame");
		return false;
	}

	// 1. Packet 파싱
	auto responsePacket = PacketFactory::Deserialize<CS_USE_ITEM_PACKET>(packet);
	if (not session->GetInventory()->hasItem(responsePacket.itemId)) {
		return false;
	}

	// 2. 아이템 효과 적용 (추후 DB 연동 및 아이템 많아지면 별도 핸들러로 분기)
	switch (responsePacket.itemId) {
	case HpPotion: {
		if (not session->IsAlive()) {
			return false;
		}

		short hp = session->_hp;
		session->_hp = std::min<short>(session->_hp + 5, session->_maxHp);

		if (hp != session->_hp) {
			session->GetInventory()->RemoveItem(HpPotion, 1);

			session->Send(PacketFactory::BuildUseItemOkPacket(HpPotion));
			session->Send(PacketFactory::BuildStatChangePacket(*session));
		}

		break;
	}

	default:
		LOG_ERR("Unknown ItemId %d", responsePacket.itemId);
		break;
	}

	return true;
}

std::shared_ptr<Service> Service::Create(std::shared_ptr<IocpCore> core, int maxSessionCount)
{
	std::shared_ptr<Service> service = std::make_shared<Service>(core, maxSessionCount);
	service->_chatManager.SetService(service);

	return service;
}