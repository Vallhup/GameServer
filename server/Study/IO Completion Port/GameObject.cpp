#include "pch.h"
#include "GameObject.h"

NPC::NPC(int id, short x, short y, std::string_view name) : GameObject(id, x, y)
{
	_type = ObjectType::Npc;
	_name = name;
	//_nextActionTime = std::chrono::steady_clock::now();
}

void NPC::WakeUp(bool force)
{
	if (force) {
		if (_isActive.exchange(true)) {
			return;
		}

		if (not _timerPending.exchange(true)) {
			RegisterTimer();
		}
	}

	else {
		if (not _isActive.exchange(true)) {
			if (not _timerPending.exchange(true)) {
				RegisterTimer();
			}
		}
	}
}

void NPC::Update()
{
	OnTimer();
}

void NPC::OnTimer()
{
	_timerPending.store(false);

	if (not _isActive.load()) {
		return;
	}

	// 일정 주기마다 진행될 AI 동작 (Random Move)
	RandomMove();

	// 1. 활성화 판단
	bool isActive{ false };
	auto service = _service.lock();
	if (nullptr != service) {
		auto visibleList = ViewListHelper::collectViewList(shared_from_this(), service);
		for (int id : visibleList) {
			if (id >= MAX_USER) continue;
			isActive = true;
			break;
		}
	}

	// 2. 움직인 후에도 Player가 근처에 있으면 다음 Event Push, 없으면 비활성화
	if (isActive) {
		RegisterTimer();
	}

	else {
		_isActive.store(false);
	}
}

void NPC::RegisterTimer()
{
	if (auto service = _service.lock()) {
		int interval = service->getRandomInterval(1000, 2000);
		service->_timerQueue.push(
			Event{ _id,
			std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(interval),
			EV_MOVE, 0 });
	}
}

void NPC::RandomMove()
{
	ServicePtr service = _service.lock();
	if (nullptr == service) {
		return;
	}

	// 1. 이동 전 viewList / Sector
	std::unordered_set<int> oldViewList = ViewListHelper::collectViewList(shared_from_this(), service);
	auto oldSector = Sector::getSector(_x, _y);

	// 2. Random Move
	switch (rand() % 4) {
	case 0: if (_x < (W_WIDTH - 1)) _x++; break;
	case 1: if (_x > 0) _x--; break;
	case 2: if (_y < (W_HEIGHT - 1)) _y++; break;
	case 3: if (_y > 0) _y--; break;
	}

	// 3. 이동 후 viewList / Sector
	std::unordered_set<int> newViewList = ViewListHelper::collectViewList(shared_from_this(), service);
	auto newSector = Sector::getSector(_x, _y);

	// 4. 이동 전후 viewList / Sector 동기화
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
}


