#include "pch.h"
#include "Session.h"

Session::Session(int sessionId, SOCKET socket) : _id(sessionId), _socket(socket)
{
}

Session::~Session()
{
	DisConnect();
}

bool Session::Recv()
{
	if (INVALID_SOCKET == _socket or not _isConnected) {
		return false;
	}

	DWORD flags{ 0 };
	DWORD bytesReceived{ 0 };

	WSABUF wsaBuf;
	wsaBuf.buf = _recvBuffer.GetWritePos();
	wsaBuf.len = _recvBuffer.GetContiguousFreeSize();

	if (wsaBuf.len == 0) {
		return true;
	}

	if (SOCKET_ERROR == WSARecv(_socket, &wsaBuf, 1, &bytesReceived, &flags, nullptr, nullptr)) {
		int error = WSAGetLastError();
		if (error == WSAEWOULDBLOCK or error == WSA_IO_PENDING) {
			return true;
		}

		else if (error == WSAECONNRESET) {
			LOG_INF("Session[%d] remote closed (10054)", _id);
		}

		else {
			LOG_ERR("Session[%d] WSARecv failed : %d", _id, error);
		}

		return false;
	}

	if (bytesReceived == 0) {
		LOG_INF("Session[%d] DisConnected", _id);
		return false;
	}

	if (not _recvBuffer.Write(nullptr, bytesReceived)) {
		LOG_ERR("Session[%d] RecvBuffer OverFlow", _id);
		return false;
	}

	// Packet 처리
	ProcessPacket();
}

bool Session::Send(const std::vector<char>& data)
{
	LOG_DBG("Session[%d] Send", _id);

	if (data.empty() or _socket == INVALID_SOCKET or not _isConnected) {
		return false;
	}

	WSABUF wsaBuf;
	wsaBuf.buf = const_cast<char*>(data.data());
	wsaBuf.len = static_cast<ULONG>(data.size());

	DWORD bytesSent{ 0 };
	if (SOCKET_ERROR == WSASend(_socket, &wsaBuf, 1, &bytesSent, 0, nullptr, nullptr)) {
		int error = WSAGetLastError();
		if (error == WSA_IO_PENDING or error == WSAEWOULDBLOCK) {
			return true;
		}

		LOG_ERR("Session[%d] WSASend failed : %d", _id, error);
		return false;
	}

	if (bytesSent != data.size()) {
		LOG_WRN("Session[%d] Partial send : %d / %d bytes", _id, bytesSent, static_cast<int>(data.size()));
	}

	return true;
}

void Session::OnConnect()
{
	LOG_INF("Client Connected : %d", _id);

	_isConnected = true;

	// TODO : Login, Init 등...
	std::string msg{ "Connect!" };
	unsigned char packetSize = static_cast<unsigned char>(msg.size() + sizeof(unsigned char));

	std::vector<char> packet(packetSize);
	memcpy(packet.data(), &packetSize, sizeof(unsigned char));
	memcpy(packet.data() + sizeof(unsigned char), msg.data(), msg.size());

	Send(packet);
}

void Session::DisConnect()
{
	if (INVALID_SOCKET != _socket) {
		shutdown(_socket, SD_BOTH);
		closesocket(_socket);
		_socket = INVALID_SOCKET;
		_isConnected = false;
	}
}

void Session::ProcessPacket()
{
	while (true) {
		if (_recvBuffer.GetUsedSize() < sizeof(unsigned char)) {
			break;
		}

		unsigned char packetSize{ 0 };
		if (not _recvBuffer.Peek(reinterpret_cast<char*>(&packetSize), sizeof(unsigned char))) {
			break;
		}

		if (_recvBuffer.GetUsedSize() < packetSize) {
			break;
		}

		std::vector<char> packetBuf(packetSize);
		if (not _recvBuffer.Read(packetBuf.data(), packetSize)) {
			LOG_ERR("Session[%d] ProcessPacket Read Failed", _id);
			break;
		}

		HandlePacket(packetBuf);
	}
}

void Session::HandlePacket(const std::vector<char>& packet)
{
	const unsigned char packetSize = packet[0];
	if (packet.size() < packetSize) {
		return;
	}

	const char packetType = packet[1];

	switch (packetType) {
	case CS_MOVE:
		HandleMovePacket(packet);
		break;

	case CS_ATTACK:
		HandleAttackPacket(packet);
		break;

	default:
		LOG_WRN("Unknown Packet Type : %d", packetType);
		break;
	}
}

void Session::HandleMovePacket(const std::vector<char>& packet)
{
	// 1. Packet 파싱
	auto move = PacketFactory::Deserialize<CS_MOVE_PACKET>(packet);

	// TODO : 유효성 검사, GameObject에서 실제 로직 실행, 전체 Client에 BroadCast
}

void Session::HandleAttackPacket(const std::vector<char>& packet)
{
	// 1. Packet 파싱
	auto attack = PacketFactory::Deserialize<CS_ATTACK_PACKET>(packet);

	// TODO : 유효성 검사, GameObject에서 실제 로직 실행, 전체 Client에 BroadCast
}
