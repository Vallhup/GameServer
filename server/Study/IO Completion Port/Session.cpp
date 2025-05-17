#include "pch.h"
#include "Session.h"

Session::~Session()
{ 
	LOG_DBG("Session %d Delete", _sessionId);
}

void Session::Send(const std::vector<char>& data)
{
	if (_state.load() == ST_FREE) {
		return;
	}

	_sendQueue.Push(data);

	if (not _isSending.exchange(true)) {
		doSend();
	}
}

void Session::doRecv()
{
	if ((ST_FREE == _state.load()) or (_socket == INVALID_SOCKET)) {
		return;
	}

	DWORD recvFlag = 0;

	auto sp = static_cast<GameSession*>(this);

	_recvOver.Init();
	_recvOver._owner = sp->shared_from_this();

	_recvOver._wsaBuf[0].buf = _recvOver._buffer.GetWritePos();
	_recvOver._wsaBuf[0].len = _recvOver._buffer.GetFreeSize();

	_pendingIoCount.fetch_add(1);
	int result = WSARecv(_socket, _recvOver._wsaBuf, 1, NULL, &recvFlag, reinterpret_cast<LPWSAOVERLAPPED>(&_recvOver), NULL);
	if (SOCKET_ERROR == result) {
		int error = WSAGetLastError();
		if (WSA_IO_PENDING != error) {
			if (error == WSAECONNRESET || error == WSAENOTCONN || error == WSAESHUTDOWN || error == WSA_OPERATION_ABORTED) {
				LOG_WRN("WSARecv disconnected/aborted: %d", error);
			}
			else {
				LOG_ERR("WSARecv failed: %d", error);
			}
		}
	}
}

void Session::doSend()
{
	if ((ST_FREE == _state.load()) or (_socket == INVALID_SOCKET)) {
		return;
	}

	constexpr size_t MAX_PACKET = 32;
	std::vector<std::shared_ptr<std::vector<char>>> packets;
	packets.reserve(MAX_PACKET);

	std::shared_ptr<std::vector<char>> sendData;
	while ((packets.size() < MAX_PACKET) and (_sendQueue.tryPop(sendData))) {
		packets.push_back(sendData);
	}

	if (packets.empty()) {
		_isSending.store(false);
		return;
	}

	auto sp = static_cast<GameSession*>(this);

	_sendOver.Init();
	_sendOver._owner = sp->shared_from_this();

	_sendOver.SetBuffers(std::move(packets));

	DWORD bytesSent{ 0 };
	_pendingIoCount.fetch_add(1);
	if (SOCKET_ERROR == WSASend(_socket, _sendOver._wsaBufs.data(), _sendOver._wsaBufs.size(), &bytesSent, 0, reinterpret_cast<LPWSAOVERLAPPED>(&_sendOver), NULL)) {
		int error = WSAGetLastError();
		if ((error == WSAECONNRESET) or (error == WSAENOTCONN) or (error == WSAESHUTDOWN)) {
			LOG_INF("Client %d disconnected", _sessionId);
		}

		else if (error != WSA_IO_PENDING) {
			LOG_ERR("WSASend failed: %d", error);
		}

		_isSending.store(false);
		Close();
		return;
	}
}

void Session::RecvCallback(DWORD numBytes)
{
	if (numBytes == 0) {
		LOG_INF("Client %d disconnected", _sessionId);
		Close();
		return;
	}

	if (not _recvOver._buffer.Write(nullptr, numBytes)) {
		LOG_ERR("RecvBuffer overflow in session %d", _sessionId);
		Close();
		return;
	}

	std::vector<char> readBuffer(numBytes);
	_recvOver._buffer.Read(readBuffer.data(), numBytes);

	ProcessPacket(readBuffer);

	if (_pendingIoCount.fetch_sub(1) == 1) {
		if (_shouldRelease) {
			if (auto service = _service.lock()) {
				auto self = static_cast<GameSession*>(this);
				service->FinalizeRelease(self->shared_from_this());
			}
		}
	}

	_recvOver._owner.reset();

	doRecv();
}

void Session::SendCallback()
{
	LOG_INF("SendCallback()");

	if (_pendingIoCount.fetch_sub(1) == 1) {
		if (_shouldRelease) {
			if (auto service = _service.lock()) {
				auto self = static_cast<GameSession*>(this);
				service->FinalizeRelease(self->shared_from_this());
			}
		}
	}

	_sendOver._owner.reset();

	doSend();
}

void Session::Close()
{
	// 이미 닫힌 Session이면 return, 닫히지 않았으면 state를 Free로 변경
	if (_state.exchange(ST_FREE) == ST_FREE) {
		return;
	}

	shutdown(_socket, SD_BOTH);
	CancelIoEx(reinterpret_cast<HANDLE>(_socket), nullptr);
	closesocket(_socket);

	_shouldRelease = true;

	if (_pendingIoCount.load() == 0) {
		if (auto service = _service.lock()) {
			auto sp = static_cast<GameSession*>(this);
			service->FinalizeRelease(sp->shared_from_this());
		}
	}

	LOG_INF("Closing Session %d", _sessionId);
}

HANDLE Session::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Session::Dispatch(ExpOver* expOver, int numOfBytes)
{
	if (ST_FREE == _state.load()) {
		return;
	}

	LOG_INF("Session Dispatch");

	switch (expOver->_operationType) {
	case OperationType::Recv:
		RecvCallback(numOfBytes);
		break;

	case OperationType::Send:
		SendCallback();
		break;

	default:
		LOG_WRN("Unknown operation in Dispatch: op=%d", expOver->_operationType);
		break;
	}
}

GameSession::GameSession() : Session(), GameObject()
{
	_type = ObjectType::Player;
}

GameSession::~GameSession()
{
	LOG_DBG("GameSession %d Delete", _id);
}

bool GameSession::ProcessPacket(const std::vector<char>& packet)
{
	if (ST_FREE == _state.load()) {
		LOG_WRN("Session state is Free");
		return false;
	}

	auto service = _service.lock();
	if (not service) {
		LOG_WRN("Service expired in CS_LOGIN");
		return false;
	}

	char packetType = packet[1];
	bool handled = false;

	switch (packetType) {
	case CS_LOGIN:	handled = HandleLogin(packet, service); break;
	case CS_MOVE :	handled = HandleMove(packet, service); break;
	case CS_CHAT :	handled = HandleChat(packet, service); break;
	default:
		LOG_WRN("Packet Type Error");
		return false;
	}

	return handled;
}

bool GameSession::HandleLogin(const std::vector<char>& packet, std::shared_ptr<Service> service)
{
	// 0. ALLOC 상태를 INGAME으로 변경
	State expect = ST_ALLOC;
	if (not _state.compare_exchange_strong(expect, ST_INGAME)) {
		LOG_WRN("Session state is not Alloc");
		return false;
	}

	// 1. packet 파싱
	auto requestPacket = PacketFactory::Deserialize<CS_LOGIN_PACKET>(packet);
	requestPacket.name[NAME_SIZE - 1] = '\0';

	// 2. Session name 설정
	_name = requestPacket.name;

	// 3. Session Container에 자기자신 등록
	service->AddObject(shared_from_this());

	auto loginPacket = PacketFactory::BuildLoginPacket(*shared_from_this());
	Send(loginPacket);

	// 4. Sector에 자기자신 등록
	service->enterSector(shared_from_this());

	// 5. OnPlayerMove 호출
	service->OnPlayerLogin(shared_from_this());

	// 6. viewList 동기화
	auto newViewList = ViewListHelper::collectViewList(shared_from_this(), service);
	auto oldViewList = ViewListHelper::updateViewList(_viewList, newViewList);

	ViewListDiff diffViewList = ViewListHelper::calcViewListDiff(oldViewList, newViewList);

	for (int addPlayer : diffViewList.addViewList) {
		auto object = service->FindObject(addPlayer);
		if (nullptr == object) {
			continue;
		}

		auto packetForSelf = PacketFactory::BuildAddPacket(*object);
		Send(packetForSelf);

		if (object->GetType() == ObjectType::Player) {
			auto target = static_pointer_cast<GameSession>(object);
			auto packetForTarget = PacketFactory::BuildAddPacket(*shared_from_this());
			target->Send(packetForTarget);
		}
	}

	LOG_DBG("Process Login Packet Success");
	return true;
}

bool GameSession::HandleMove(const std::vector<char>& packet, std::shared_ptr<Service> service)
{
	// 0. INGAME이 아니면 실행 X
	if (_state.load() != ST_INGAME) {
		LOG_WRN("Session state is not Ingame");
		return false;
	}

	// 1. packet 파싱
	auto requestPacket = PacketFactory::Deserialize<CS_MOVE_PACKET>(packet);
	_lastMoveTime = requestPacket.move_time;

	auto oldSector = Sector::getSector(_x, _y);

	// 2. 자신의 Pos 업데이트
	switch (requestPacket.direction) {
	case UP:	if (_y > 0)				_y--; break;
	case DOWN:	if (_y < W_HEIGHT - 1)	_y++; break;
	case LEFT:	if (_x > 0)				_x--; break;
	case RIGHT: if (_x < W_WIDTH - 1)	_x++; break;
	}

	auto newSector = Sector::getSector(_x, _y);

	// 3. Sector 동기화
	if (oldSector != newSector) {
		service->leaveSector(shared_from_this(), oldSector.first, oldSector.second);
		service->enterSector(shared_from_this(), newSector.first, newSector.second);
	}

	// 4. viewList 동기화
	auto newViewList = ViewListHelper::collectViewList(shared_from_this(), service);
	auto oldViewList = ViewListHelper::updateViewList(_viewList, newViewList);

	ViewListDiff diffViewList = ViewListHelper::calcViewListDiff(oldViewList, newViewList);
	
	// 5. OnPlayerMove 호출
	service->OnPlayerMove(shared_from_this());

	// 6. 각 Player에게 Packet Send
	auto myPacket = PacketFactory::BuildMovePacket(*this);
	Send(myPacket);

	for (int addPlayer : diffViewList.addViewList) {
		auto object = service->FindObject(addPlayer);
		if (nullptr == object) {
			continue;
		}

		auto packetForSelf = PacketFactory::BuildAddPacket(*object);
		Send(packetForSelf);

		if (object->GetType() == ObjectType::Player) {
			auto target = static_pointer_cast<GameSession>(object);
			auto packetForTarget = PacketFactory::BuildAddPacket(*shared_from_this());
			target->Send(packetForTarget);
		}
	}

	for (int movePlayer : diffViewList.moveViewList) {
		auto object = service->FindObject(movePlayer);
		if (nullptr == object) {
			continue;
		}

		auto packetForSelf = PacketFactory::BuildMovePacket(*object);
		Send(packetForSelf);

		if (object->GetType() == ObjectType::Player) {
			auto target = static_pointer_cast<GameSession>(object);
			auto packetForTarget = PacketFactory::BuildMovePacket(*shared_from_this());
			target->Send(packetForTarget);
		}
	}

	for (int removePlayer : diffViewList.removeViewList) {
		auto object = service->FindObject(removePlayer);
		if (nullptr == object) {
			continue;
		}

		auto packetForSelf = PacketFactory::BuildRemovePacket(*object);
		Send(packetForSelf);

		if (object->GetType() == ObjectType::Player) {
			auto target = static_pointer_cast<GameSession>(object);
			auto packetForTarget = PacketFactory::BuildRemovePacket(*shared_from_this());
			target->Send(packetForTarget);
		}
	}

	LOG_DBG("Process Move Packet Success");
	return true;
}

bool GameSession::HandleChat(const std::vector<char>& packet, std::shared_ptr<Service> service)
{
	if (_state.load() != ST_INGAME) {
		LOG_WRN("Session state is not Ingame");
		return false;
	}

	auto requestPacket = PacketFactory::Deserialize<CS_CHAT_PACKET>(packet);
	requestPacket.message[CHAT_SIZE - 1] = '\0';

	service->OnChatRequest(_id, requestPacket.message);

	LOG_DBG("Process Chat Packet Success");
	return true;
}
