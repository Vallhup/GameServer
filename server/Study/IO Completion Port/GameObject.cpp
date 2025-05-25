#include "pch.h"
#include "GameObject.h"

GameObject::GameObject()
	: _id(-1), _x(rand() % 2000), _y(rand() % 2000), _type(Player), _damage(10),
	_lastMoveTime(0), _lastAttackTime(0), _hp(1), _maxHp(10), _exp(10), _level(1),
	_defaultX(_x), _defaultY(_y)
{
}

GameObject::GameObject(int id, short x, short y)
	: _id(id), _x(x), _y(y), _type(Player), _damage(10),
	_lastMoveTime(0), _lastAttackTime(0), _hp(10), _maxHp(10), _exp(10), _level(1),
	_defaultX(_x), _defaultY(_y)
{
}

NPC::NPC(int id, short x, short y, std::string_view name) : GameObject(id, x, y)
{
	_type = ObjectType::Npc;
	_name = name;
}

void NPC::WakeUp(bool force)
{
	if (force) {
		if (_isActive.exchange(true)) {
			return;
		}

		if (_isAlive.load()) {
			_movePending.store(false);
			RegisterTimer();
		}
	}

	else {
		if (not _isActive.exchange(true)) {
			if (_isAlive.load()) {
				_movePending.store(false);
				RegisterTimer();
			}
		}
	}
}

void NPC::OnMove()
{
	_movePending.store(false);

	if (not _isAlive.load()) {
		return;
	}

	if (not _isActive.load()) {
		return;
	}

	auto service = _service.lock();
	if (nullptr == service) {
		return;
	}

	APos npcPos{ _x, _y };
	APos defaultPos{ _defaultX, _defaultY };

	// 1. 주변 Player 탐색
	bool agro{ false };
	auto visibleList = ViewListHelper::collectViewList(shared_from_this(), service);

	APos targetPos;

	// 2. 상태 결정
	for (int id : visibleList) {
		if (id >= MAX_USER) continue;

		auto player = static_pointer_cast<GameSession>(service->FindObject(id));
		if ((nullptr != player) and (player->IsAlive())) {
			int dx = std::abs(player->GetX() - _x);
			int dy = std::abs(player->GetY() - _y);

			// Player가 11 x 11 안에 들어오면 Agro 상태
			if ((dx <= 5) and (dy <= 5)) {
				int dist = dx + dy;
				targetPos = { player->GetX(), player->GetY() };
				agro = true;
				break;
			}
		}
	}

	std::unordered_set<int> newViewList;

	// 3-1. Agro 상태면 AStar로 Player 쫓아가기
	if (agro) {
		newViewList = AStarMove(service, npcPos, targetPos);
	}

	// 3-2. Agro 상태 아니면 Random Move
	else {
		newViewList = RandomMove(service);
	}

	bool isActive{ false };
	for (int id : newViewList) {
		if (id >= MAX_USER) continue;
		isActive = true;
		break;
	}

	// 4. 움직인 후에도 Player가 근처에 있으면 다음 Event Push, 없으면 비활성화
	if (isActive) {
		RegisterTimer();
	}

	else {
		_isActive.store(false);
	}
}

void NPC::OnHeal()
{
	_movePending.store(false);
	_healPending.store(false);

	if (not _isAlive.load()) {
		_hp = _maxHp;
		_x = _defaultX;
		_y = _defaultY;
		_isAlive.store(true);

		if (auto service = _service.lock()) {
			auto visible = service->collectVisibleObjects(shared_from_this());
			for (int id : visible) {
				if (id >= MAX_USER) continue;
				
				auto player = static_pointer_cast<GameSession>(service->FindObject(id));
				if (nullptr == player) continue;

				// ViewList 동기화
				auto newViewList = ViewListHelper::collectViewList(player, service);
				auto oldViewList = ViewListHelper::updateViewList(player->_viewList, newViewList);

				auto snapShot = player->_viewList.load();
				if ((_isAlive.load()) and snapShot->contains(GetId())) {
					auto addPacket = PacketFactory::BuildAddPacket(*this);
					player->Send(addPacket);
				}
			}
		}

		RegisterTimer();
	}
}

void NPC::RegisterTimer()
{
	if (_movePending.exchange(true)) {
		LOG_ERR("NPC %d: RegisterTimer skipped, movePending already true", _id);
		return;
	}

	if (not _isAlive.load()) {
		LOG_ERR("NPC %d: RegisterTimer blocked, isAlive == false", _id);
		return;
	}

	if (auto service = _service.lock()) {
		int interval = service->getRandomInterval(1000, 2000);
		service->_timerQueue.push(
			Event{ _id,
			std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(interval),
			EV_MOVE, 0 });
	}
}

void NPC::RegisterSuicideTimer()
{
	if (auto service = _service.lock()) {
		service->_timerQueue.push(Event{
			_id,
			std::chrono::high_resolution_clock::now() + std::chrono::seconds(20),
			EV_ATTACK,  // EV_ATTACK == 자살 시뮬레이션
			_id         // 자기 자신을 대상으로 공격
			});
	}
}

std::unordered_set<int> NPC::RandomMove(std::shared_ptr<Service> service)
{
	// 1. 이동 전 viewList / Sector
	std::unordered_set<int> oldViewList = ViewListHelper::collectViewList(shared_from_this(), service);
	auto oldSector = Sector::getSector(_x, _y);

	// 2. 이동 범위 제한 (Default Pos를 중심으로 20 x 20)
	int minX = std::max<int>(0, _defaultX - 10);
	int maxX = std::min<int>(W_WIDTH - 1, _defaultX + 10);

	int minY = std::max<int>(0, _defaultY - 10);
	int maxY = std::min<int>(W_HEIGHT - 1, _defaultY + 10);

	// 3. 범위 내에서 Random Move
	for (int i = 0; i < 10; ++i) {
		short randX = minX + (rand() % (maxX - minX + 1));
		short randY = minY + (rand() % (maxY - minY + 1));

		if (!service->_navigationMap[randY][randX]) continue;

		APos startPos{ _x, _y };
		APos targetPos{ randX, randY };

		if (startPos == targetPos) continue;

		auto path = AStar(service->_navigationMap, startPos, targetPos);
		if (path.size() > 1) {
			APos next = path[1];
			_x = next.x;
			_y = next.y;
			break;
		}
	}

	// 4. 이동 후 viewList / Sector
	std::unordered_set<int> newViewList = ViewListHelper::collectViewList(shared_from_this(), service);
	auto newSector = Sector::getSector(_x, _y);

	// 5. 이동 전후 viewList / Sector 동기화
	ViewListDiff diffViewList = ViewListHelper::calcViewListDiff(oldViewList, newViewList);

	if (oldSector != newSector) {
		service->leaveSector(shared_from_this(), oldSector.first, oldSector.second);
		service->enterSector(shared_from_this(), newSector.first, newSector.second);
	}

	// 5. 각 Player에게 Packet Send (Service -> Session이 처리)
	for (int addPlayer : diffViewList.addViewList) {
		if (addPlayer >= MAX_USER) continue;
		auto target = static_pointer_cast<GameSession>(service->FindObject(addPlayer));
		if (nullptr != target) {
			auto packet = PacketFactory::BuildAddPacket(*this);
			target->Send(packet);
		}
	}

	for (int movePlayer : diffViewList.moveViewList) {
		if (movePlayer >= MAX_USER) continue;
		auto target = static_pointer_cast<GameSession>(service->FindObject(movePlayer));
		if (nullptr != target) {
			auto packet = PacketFactory::BuildMovePacket(*this);
			target->Send(packet);
		}
	}

	for (int removePlayer : diffViewList.removeViewList) {
		if (removePlayer >= MAX_USER) continue;
		auto target = static_pointer_cast<GameSession>(service->FindObject(removePlayer));
		if (nullptr != target) {
			auto packet = PacketFactory::BuildRemovePacket(*this);
			target->Send(packet);
		}
	}

	return newViewList;
}

std::unordered_set<int> NPC::AStarMove(std::shared_ptr<Service> service, APos npcPos, APos targetPos)
{
	std::unordered_set<int> newViewList;

	// Temp : service에서 Map Data 파싱 필요
	auto path = AStar(service->_navigationMap, npcPos, targetPos);
	if (path.size() > 1) {
		// 이동 전 ViewList / Sector
		auto oldViewList = ViewListHelper::collectViewList(shared_from_this(), service);
		auto oldSector = Sector::getSector(_x, _y);

		// 이동
		APos next = path[1];
		_x = next.x;
		_y = next.y;

		// 이동 후 ViewList / Sector
		newViewList = ViewListHelper::collectViewList(shared_from_this(), service);
		auto newSector = Sector::getSector(_x, _y);

		// 이동 전후 ViewList / Sector 동기화
		ViewListDiff viewListDiff = ViewListHelper::calcViewListDiff(oldViewList, newViewList);

		if (oldSector != newSector) {
			service->leaveSector(shared_from_this(), oldSector.first, oldSector.second);
			service->enterSector(shared_from_this(), newSector.first, newSector.second);
		}

		// 각 Player에게 Packet Send
		for (int addPlayer : viewListDiff.addViewList) {
			if (addPlayer >= MAX_USER) continue;
			auto target = static_pointer_cast<GameSession>(service->FindObject(addPlayer));
			if (nullptr != target) {
				auto packet = PacketFactory::BuildAddPacket(*this);
				target->Send(packet);
			}
		}

		for (int movePlayer : viewListDiff.moveViewList) {
			if (movePlayer >= MAX_USER) continue;
			auto target = static_pointer_cast<GameSession>(service->FindObject(movePlayer));
			if (nullptr != target) {
				auto packet = PacketFactory::BuildMovePacket(*this);
				target->Send(packet);
			}
		}

		for (int removePlayer : viewListDiff.removeViewList) {
			if (removePlayer >= MAX_USER) continue;
			auto target = static_pointer_cast<GameSession>(service->FindObject(removePlayer));
			if (nullptr != target) {
				auto packet = PacketFactory::BuildRemovePacket(*this);
				target->Send(packet);
			}
		}
	}

	else {
		newViewList = ViewListHelper::collectViewList(shared_from_this(), service);
	}

	return newViewList;
}

HANDLE NPC::GetHandle()
{
	return INVALID_HANDLE_VALUE;
}

void NPC::Dispatch(ExpOver* expOver, int numBytes)
{
	switch (expOver->_operationType) {
	case NpcMove:
		OnMove();
		break;

	case NpcHeal:
		OnHeal();
		break;

	//case NpcAttack: {
	//	// 테스트용 자살
	//	if (_isAlive.load()) {
	//		_hp = 0;
	//		_isAlive.store(false);

	//		if (auto service = _service.lock()) {
	//			auto visible = service->collectVisibleObjects(shared_from_this());
	//			for (int id : visible) {
	//				if (id >= MAX_USER) continue;
	//				auto player = std::static_pointer_cast<GameSession>(service->FindObject(id));
	//				if (player && player->_viewList.contains(_id)) {
	//					auto removePacket = PacketFactory::BuildRemovePacket(*this);
	//					player->Send(removePacket);
	//				}
	//			}

	//			service->_timerQueue.push(Event{
	//				_id,
	//				std::chrono::high_resolution_clock::now() + std::chrono::seconds(10),
	//				EV_HEAL,
	//				0
	//				});
	//		}
	//	}
	//	break;
	//}

	default:
		LOG_ERR("Unknown NPC OperationType: %d", expOver->_operationType);
		break;
	}
}