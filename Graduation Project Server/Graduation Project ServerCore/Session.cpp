#include "pch.h"
#include "Session.h"

Session::Session(int id, SOCKET socket) : _id(id), _socket(socket)
{
	_connected = true;
	_character = nullptr;
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
	if (not _connected.load() or _socket == INVALID_SOCKET) {
		return;
	}

	DWORD recvFlag{ 0 };
	int wsaBufCount = _recvOver.SetBuffers();
	int result = WSARecv(_socket, _recvOver._wsaBuf.data(), wsaBufCount, NULL, &recvFlag, reinterpret_cast<LPWSAOVERLAPPED>(&_recvOver), NULL);
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

void Session::RegisterSend(const std::vector<char>& data)
{
	if (not _connected.load() or _socket == INVALID_SOCKET) {
		DisConnect();
		return;
	}

	_sendQueue.push(data);

	bool expected{ false };
	if (_isSending.compare_exchange_strong(expected, true)) {
		InternalSend();
	}
}

void Session::ProcessRecv(DWORD numBytes)
{
	if (numBytes == 0 or not _recvOver._buffer.Write(nullptr, numBytes)) {
		DisConnect();
		return;
	}

	std::vector<char> readBuffer(numBytes);
	_recvOver._buffer.Read(readBuffer.data(), numBytes);

	if (_packetHandler) {
		_packetHandler(_id, std::move(readBuffer));
	}

	RegisterRecv();
}

void Session::ProcessSend()
{
	InternalSend();
}

void Session::DisConnect()
{
	bool expected{ true };
	if (_connected.compare_exchange_strong(expected, false)) {
		shutdown(_socket, SD_BOTH);
		CancelIoEx(GetHandle(), nullptr);
		closesocket(_socket);
		_socket = INVALID_SOCKET;
	}
}

void Session::InternalSend()
{
	if (not _connected.load() or _socket == INVALID_SOCKET) {
		DisConnect();
		return;
	}

	std::vector<std::vector<char>> packets;
	packets.reserve(MAX_PACKET);

	std::vector<char> sendData;
	while ((packets.size() < MAX_PACKET) and (_sendQueue.try_pop(sendData))) {
		packets.push_back(sendData);
	}

	if (packets.empty()) {
		_isSending.store(false);
		return;
	}

	auto sendOver = new SendOver;
	sendOver->SetBuffers(std::move(packets));

	DWORD bytesSent{ 0 };
	int result = WSASend(_socket, sendOver->_wsaBufs.data(), static_cast<DWORD>(sendOver->_wsaBufs.size()), &bytesSent, 0, reinterpret_cast<LPWSAOVERLAPPED>(sendOver), NULL);
	if (SOCKET_ERROR == result) {
		int error = WSAGetLastError();
		if ((error == WSAECONNRESET) or (error == WSAENOTCONN) or (error == WSAESHUTDOWN)) {
			LOG_INF("Session[%d] disconnected", _id);
		}

		else if (error != WSA_IO_PENDING) {
			LOG_ERR("WSASend failed: %d", error);
		}

		_isSending.store(false);
		delete sendOver;

		DisConnect();
		return;
	}
}