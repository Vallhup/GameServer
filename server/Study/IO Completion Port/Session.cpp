#include "pch.h"
#include "Session.h"

Session::~Session()
{ 
	Close();
}

void Session::Send(const std::vector<char>& data)
{
	_sendQueue.Push(data);

	if (not _isSending.exchange(true)) {
		doSend();
	}
}

void Session::doRecv()
{
	DWORD recvFlag = 0;

	_recvOver.Init();

	if(_recvOver._owner == nullptr)
		_recvOver._owner = shared_from_this();
	
	_recvOver._wsaBuf[0].buf = _recvOver._buffer.GetWritePos();
	_recvOver._wsaBuf[0].len = _recvOver._buffer.GetFreeSize();

	int result = WSARecv(_socket, _recvOver._wsaBuf, 1, NULL, &recvFlag, reinterpret_cast<LPWSAOVERLAPPED>(&_recvOver), NULL);
	if (SOCKET_ERROR == result) {
		int error = WSAGetLastError();
		if (WSA_IO_PENDING != error) {
			LOG_ERR("WSARecv failed: %d", error);
		}
	}
}

void Session::doSend()
{
	constexpr size_t MAX_PACKET = 16;

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

	_sendOver.Init();

	if (_sendOver._owner == nullptr)
		_sendOver._owner = shared_from_this();

	_sendOver.SetBuffers(std::move(packets));

	DWORD bytesSent{ 0 };
	if (SOCKET_ERROR == WSASend(_socket, _sendOver._wsaBufs.data(), _sendOver._wsaBufs.size(), &bytesSent, 0, reinterpret_cast<LPWSAOVERLAPPED>(&_sendOver), NULL)) {
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING) {
			LOG_ERR("WSASend failed: %d", error);
			_isSending.store(false);
		}
	}
}

void Session::RecvCallback(DWORD numBytes)
{
	if (not _recvOver._buffer.Write(nullptr, numBytes)) {
		LOG_ERR("RecvBuffer overflow in session %d", _id);
		Close();
		return;
	}

	std::vector<char> readBuffer(numBytes);
	_recvOver._buffer.Read(readBuffer.data(), numBytes);

	ProcessPacket(readBuffer);
	
	doRecv();
}

void Session::SendCallback()
{
	LOG_INF("SendCallback()");
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

	_recvOver._owner.reset();
	_sendOver._owner.reset();

	closesocket(_socket);

	if (auto service = _service.lock()) {
		service->ReleaseSession(static_pointer_cast<Session>(shared_from_this()));
	}

	LOG_INF("Closing Session %d", _id);
}

HANDLE Session::GetHandle()
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Session::Dispatch(ExpOver* expOver, int numOfBytes)
{
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

GameSession::~GameSession()
{
	LOG_DBG("GameSession Delete");

	// TODO : 추후 자원 해제가 필요하게 되면 추가
}

bool GameSession::ProcessPacket(const std::vector<char>& packet)
{
	if (_state.load() == ST_FREE) {
		LOG_WRN("Session state is Free");
		return false;
	}

	auto service = _service.lock();
	if (not service) {
		LOG_WRN("Service expired in CS_LOGIN");
		return false;
	}

	char packetType = packet[1];

	switch (packetType) {
	case CS_LOGIN: {
		// 0. ALLOC 상태를 INGAME으로 변경
		State expect = ST_ALLOC;
		if (not _state.compare_exchange_strong(expect, ST_INGAME)) {
			LOG_WRN("Session state is not Alloc");
			return false;
		}

		// 1. packet 파싱
		auto requestPacket = Deserialize<CS_LOGIN_PACKET>(packet);

		// 2. Session name 설정
		requestPacket.name[NAME_SIZE - 1] = '\0';
		_name = requestPacket.name;

		// 3. Session Container에 자기자신 등록
		service->AddSession(static_pointer_cast<Session>(shared_from_this()));

		// 4. 내 client에게 LOGIN_INFO Send
		auto self = static_pointer_cast<GameSession>(shared_from_this());
		sendLoginPacket(self);

		// 5. 다른 client들에게 ADD_PLAYER Send
		for (auto& [id, session] : service->_sessions) {
			auto target = static_pointer_cast<GameSession>(session.load());
			if (nullptr != target) {
				// 자기 자신을 제외하고 Broadcast
				if (target != self) {
					target->sendAddPlayerPacket(self);
				}

				// 6. 내 client에게 기존 접속자 Send
				else {
					self->sendAddPlayerPacket(target);
				}
			}
		}

		break;
	}
	case CS_MOVE: {
		// 0. INGAME이 아니면 실행 X
		if (_state.load() != ST_INGAME) {
			LOG_WRN("Session state is not Ingame");
			return false;
		}

		// 1. packet 파싱
		auto requestPacket = Deserialize<CS_MOVE_PACKET>(packet);

		// 2. 자신의 Pos 업데이트
		switch (requestPacket.direction) {
		case UP   : if (_pos._yPos > 0) _pos._yPos--; break;
		case DOWN : if (_pos._yPos < W_HEIGHT - 1) _pos._yPos++; break;
		case LEFT : if (_pos._xPos > 0) _pos._xPos--; break;
		case RIGHT: if (_pos._xPos < W_WIDTH - 1) _pos._xPos++; break;
		}

		// 3. 업데이트 된 Pos Broadcast
		auto self = static_pointer_cast<GameSession>(shared_from_this());

		for (auto& [id, session] : service->_sessions) {
			auto target = static_pointer_cast<GameSession>(session.load());
			if (nullptr != target) {
				target->sendMovePacket(self);
			}
		}

		break;
	}
	default:
		LOG_WRN("Packet Type Error");
		return false;
	}

	LOG_DBG("ProcessPacket Success");
	return true;
}

void GameSession::sendAddPlayerPacket(const std::shared_ptr<GameSession>& target)
{
	SC_ADD_PLAYER_PACKET add;
	add.id = static_cast<short>(target->GetSessionId());
	add.size = sizeof(add);
	add.type = SC_ADD_PLAYER;
	add.x = target->_pos._xPos;
	add.y = target->_pos._yPos;
	strcpy_s(add.name, NAME_SIZE, target->_name.c_str());

	Send(Serialize(add));
}

void GameSession::sendMovePacket(const std::shared_ptr<GameSession>& target)
{
	SC_MOVE_PLAYER_PACKET move;
	move.id = static_cast<short>(target->GetSessionId());
	move.size = sizeof(move);
	move.type = SC_MOVE_PLAYER;
	move.x = target->_pos._xPos;
	move.y = target->_pos._yPos;
	move.move_time = 0;

	Send(Serialize(move));
}

void GameSession::sendRemovePacket(const std::shared_ptr<GameSession>& target)
{
	SC_REMOVE_PLAYER_PACKET remove;
	remove.id = static_cast<short>(target->GetSessionId());
	remove.size = sizeof(remove);
	remove.type = SC_REMOVE_PLAYER;

	Send(Serialize(remove));
}

void GameSession::sendLoginPacket(const std::shared_ptr<GameSession>& target)
{
	SC_LOGIN_INFO_PACKET login;
	login.id = static_cast<short>(target->GetSessionId());
	login.size = sizeof(login);
	login.type = SC_LOGIN_INFO;
	login.x = target->_pos._xPos;
	login.y = target->_pos._yPos;

	Send(Serialize(login));
}
