#include "pch.h"
#include "Session.h"

Session::~Session()
{ 
	LOG_DBG("Session %d Delete", _sessionId);
}

void Session::Send(const std::vector<char>& data)
{
	if (_state.load() == ST_FREE) {
		Close();
		return;
	}

	auto buf = std::make_shared<std::vector<char>>(data);
	_sendQueue.push(buf);

	bool expected{ false };
	if (_isSending.compare_exchange_strong(expected, true)) {
		doSend();
	}
}

void Session::doRecv()
{
	if ((ST_FREE == _state.load()) or (_socket == INVALID_SOCKET)) {
		Close();
		return;
	}

	DWORD recvFlag = 0;

	auto sp = static_cast<GameSession*>(this);

	_recvOver.Init();
	_recvOver._owner = sp->shared_from_this();
	int wsaBufCount = _recvOver.PrepareWSABufs();

	_pendingIoCount.fetch_add(1);
	int result = WSARecv(_socket, _recvOver._wsaBuf, wsaBufCount, NULL, &recvFlag, reinterpret_cast<LPWSAOVERLAPPED>(&_recvOver), NULL);
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
		Close();
		return;
	}

	constexpr size_t MAX_PACKET = 32;
	std::vector<std::shared_ptr<std::vector<char>>> packets;
	packets.reserve(MAX_PACKET);

	std::shared_ptr<std::vector<char>> sendData;
	while ((packets.size() < MAX_PACKET) and (_sendQueue.try_pop(sendData))) {
		packets.push_back(std::move(sendData));
	}

	if (packets.empty()) {
		_isSending.store(false);
		return;
	}

	auto sp = static_cast<GameSession*>(this);
	auto ov = new SendOver();

	ov->Init();
	ov->_owner = sp->shared_from_this();
	ov->SetBuffers(std::move(packets));

	DWORD bytesSent{ 0 };
	_pendingIoCount.fetch_add(1);
	if (SOCKET_ERROR == WSASend(_socket, ov->_wsaBufs.data(), static_cast<DWORD>(ov->_wsaBufs.size()), &bytesSent, 0, reinterpret_cast<LPWSAOVERLAPPED>(ov), NULL)) {
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

	if (auto service = _service.lock()) {
		auto sp = static_cast<GameSession*>(this);
		service->FinalizeRelease(sp->shared_from_this());
	}

	LOG_INF("Closing Session %d", _sessionId);
}

HANDLE Session::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

void GameSession::Dispatch(ExpOver* expOver, int numOfBytes)
{
	if (ST_FREE == _state.load()) {
		Close();
		return;
	}

	LOG_INF("Session Dispatch");

	switch (expOver->_operationType) {
	case OperationType::Recv:
		RecvCallback(numOfBytes);
		break;

	case OperationType::Send:
		SendCallback();
		expOver->_owner.reset();
		delete static_cast<SendOver*>(expOver);
		break;

	case OperationType::Heal:
		OnHeal();


	default:
		LOG_WRN("Unknown operation in Dispatch: op=%d", expOver->_operationType);
		break;
	}
}

GameSession::GameSession() : Session(), GameObject()
{
	_type = ObjectType::Player;
	_viewList.store(std::make_shared<std::unordered_set<int>>());
	_inventory = nullptr;
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
	case CS_LOGIN:
	case CS_LOGOUT:
	case CS_MOVE :
	case CS_ATTACK:
	case CS_CHAT :
	case CS_PARTY_REQUEST:
	case CS_PARTY_RESPONSE:
	case CS_PARTY_LEAVE:
	case CS_USE_ITEM:
		handled = service->OnPacket(shared_from_this(), packet);
		break;

	default:
		LOG_WRN("Packet Type Error");
		return false;
	}

	return handled;
}

void GameSession::OnHeal()
{
	if ((not _isAlive) or (ST_INGAME != _state.load())) {
		return;
	}

	auto service = _service.lock();
	if (nullptr == service) {
		return;
	}

	// 1. 이미 maxHp면 다음 Event Push하고 끝
	if (_hp == _maxHp) {
		service->_timerQueue.push(Event{ _id,
			std::chrono::high_resolution_clock::now() + std::chrono::seconds(5),
			EV_PLAYER_HEAL, 0 });
		return;
	}

	// 2. maxHp의 10만큼 회복
	_hp = std::min<short>(_hp + (_maxHp / 10), _maxHp);

	// 3. Stat Update
	Send(PacketFactory::BuildStatChangePacket(*this));
	
	// 4, 다음 Event Push
	service->_timerQueue.push(Event{ _id,
		std::chrono::high_resolution_clock::now() + std::chrono::seconds(5),
		EV_PLAYER_HEAL, 0 });
}

std::shared_ptr<GameSession> GameSession::ConsumePendingPartyRequester()
{
	auto temp = _pendingPartyRequester.load();
	_pendingPartyRequester.store(std::weak_ptr<GameSession>{});

	return temp.lock();
}
