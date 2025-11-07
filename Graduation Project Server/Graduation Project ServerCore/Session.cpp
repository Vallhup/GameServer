#include "pch.h"
#include "Session.h"

Session::Session(int id, SOCKET socket, SessionManager* owner) 
	: _id(id), _socket(socket), _owner(owner), _pendingIoCount(0), _shouldRelease(false), _state(SessionState::ST_ALLOC), _character(nullptr)
{
}

Session::~Session()
{
	DisConnect();
}

HANDLE Session::GetHandle() const
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Session::Dispatch(ExpOver* expOver, int numOfBytes)
{
	if (expOver->CheckOpType(OperationType::Recv)) {
		ProcessRecv(numOfBytes);
	}

	else if (expOver->CheckOpType(OperationType::Send)) {
		ProcessSend();
	}
}

void Session::RegisterRecv()
{
	if (_state.load() == SessionState::ST_FREE or _socket == INVALID_SOCKET) {
		DisConnect();
		return;
	}

	DWORD recvFlag{ 0 };
	int wsaBufCount = _recvOver.SetBuffers();

	_pendingIoCount.fetch_add(1);
	int result = WSARecv(_socket, _recvOver._wsaBuf.data(), wsaBufCount, NULL, &recvFlag, static_cast<LPWSAOVERLAPPED>(&_recvOver), NULL);
	if (SOCKET_ERROR == result) {
		int error = WSAGetLastError();
		if (WSA_IO_PENDING != error) {
			if (_pendingIoCount.fetch_sub(1) == 1) {
				if (_shouldRelease) {
					_owner->RemoveSession(_id);
				}
			}

			if (error == WSAECONNRESET || error == WSAENOTCONN || error == WSAESHUTDOWN || error == WSA_OPERATION_ABORTED) {
				LOG_WRN("WSARecv disconnected/aborted: %d", error);
			}

			else {
				LOG_ERR("WSARecv failed: %d", error);
			}

			DisConnect();
		}
	}
}

void Session::RegisterSend(const std::vector<char>& data)
{
	if (_state.load() == SessionState::ST_FREE or _socket == INVALID_SOCKET) {
		DisConnect();
		return;
	}

	_sendQueue.push(data);
}

void Session::ProcessRecv(DWORD numBytes)
{
	if (numBytes == 0) {
		if (_pendingIoCount.fetch_sub(1) == 1) {
			if (_shouldRelease.load()) {
				_owner->RemoveSession(_id);
			}
		}

		LOG_INF("Session[%d] DisConnected", _id);
		DisConnect();
		return;
	}

	if (not _recvOver._buffer.Write(nullptr, numBytes)) {
		LOG_ERR("RecvBuffer overflow in Session[%d]", _id);
		DisConnect();
		return;
	}

	while (true) {
		if (_recvOver._buffer.GetUsedSize() < sizeof(uint16_t)) {
			break;
		}

		uint16_t packetSize{ 0 };
		if (not _recvOver._buffer.Peek(&packetSize, sizeof(packetSize))) {
			break;
		}

		if (_recvOver._buffer.GetUsedSize() < sizeof(packetSize) + packetSize) {
			break;
		}

		_recvOver._buffer.Read(nullptr, sizeof(packetSize));

		std::vector<char> readBuffer(packetSize);
		_recvOver._buffer.Read(readBuffer.data(), packetSize);

		if (_packetHandler) {
			_packetHandler(_id, std::move(readBuffer));
		}
	}

	if (_pendingIoCount.fetch_sub(1) == 1) {
		if (_shouldRelease.load()) {
			_owner->RemoveSession(_id);
		}
	}

	RegisterRecv();
}

void Session::ProcessSend()
{
	if (_pendingIoCount.fetch_sub(1) == 1) {
		if (_shouldRelease.load()) {
			_owner->RemoveSession(_id);
		}
	}
}

void Session::DisConnect()
{
	LOG_DBG("Session[%d] DisConnect", _id);

	if (_shouldRelease.exchange(true)) return;
	if (_state.exchange(SessionState::ST_FREE) != SessionState::ST_FREE) {
		shutdown(_socket, SD_BOTH);
		CancelIoEx(GetHandle(), nullptr);
		closesocket(_socket);
		_socket = INVALID_SOCKET;

		if (_pendingIoCount.load() == 0) {
			_owner->RemoveSession(_id);
		}

		return;
	}
}

void Session::InternalSend()
{
	if (_state.load() == SessionState::ST_FREE or _socket == INVALID_SOCKET) {
		DisConnect();
		return;
	}

	std::vector<std::vector<char>> packets;
	packets.reserve(MAX_PACKET);

	std::vector<char> sendData;
	while ((packets.size() < MAX_PACKET) and (_sendQueue.try_pop(sendData))) {
		if (not sendData.empty()) {
			packets.push_back(sendData);
		}
	}

	if (packets.empty()) {
		_isSending.store(false);
		return;
	}

	auto sendOver = _owner->GetSendOver();
	sendOver->SetBuffers(std::move(packets));

	DWORD bytesSent{ 0 };
	_pendingIoCount.fetch_add(1);
	int result = WSASend(_socket, sendOver->_wsaBufs.data(), static_cast<DWORD>(sendOver->_wsaBufs.size()), &bytesSent, 0, static_cast<LPWSAOVERLAPPED>(sendOver), NULL);
	if (SOCKET_ERROR == result) {
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING) {
			if (_pendingIoCount.fetch_sub(1) == 1) {
				if (_shouldRelease.load()) {
					_owner->RemoveSession(_id);
				}
			}

			if ((error == WSAECONNRESET) or (error == WSAENOTCONN) or (error == WSAESHUTDOWN)) {
				LOG_INF("Session[%d] disconnected", _id);
			}

			_isSending.store(false);
			if (_owner) {
				_owner->ReleaseSendOver(sendOver);
			}

			DisConnect();
			return;
		}
	}
}