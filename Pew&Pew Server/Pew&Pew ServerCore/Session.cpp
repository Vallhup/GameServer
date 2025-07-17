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
		LOG_DBG("Session[%d] Recv Error", _id);
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

	return true;
}

bool Session::Send(const std::vector<char>& data)
{
	LOG_DBG("Session[%d] Send", _id);

	if (data.empty() or _socket == INVALID_SOCKET or not _isConnected) {
		return false;
	}

	_sendQueue.push(data);

	bool expected{ false };
	if (_isSending.compare_exchange_strong(expected, true)) {
		return InternalSend();
	}

	return false;
}

bool Session::InternalSend()
{
	constexpr size_t MAX_PACKET{ 32 };
	std::vector<std::vector<char>> packets;
	packets.reserve(MAX_PACKET);

	std::vector<char> sendData;
	while (packets.size() < MAX_PACKET and _sendQueue.try_pop(sendData)) {
		packets.push_back(sendData);
	}

	if (packets.empty()) {
		_isSending.store(false);

		if (not _sendQueue.empty() and _isSending.exchange(true) == false) {
			return InternalSend();
		}

		return true;
	}

	std::vector<WSABUF> wsaBufs;
	wsaBufs.reserve(packets.size());
		
	for (const auto& packet : packets) {
		wsaBufs.emplace_back(static_cast<ULONG>(packet.size()), const_cast<char*>(packet.data()));
	}

	DWORD bytesSent{ 0 };
	if (SOCKET_ERROR == WSASend(_socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), &bytesSent, 0, 0, 0)) {
		int error = WSAGetLastError();
		if (error == WSAECONNRESET or error == WSAENOTCONN or error == WSAESHUTDOWN) {
			LOG_INF("Session[%d] DisConencted", _id);
		}

		else if (error != WSA_IO_PENDING) {
			LOG_ERR("Session[%d] WSASend failed : %d", _id, error);
		}

		_isSending.store(false);
		DisConnect();
		return false;
	}

	// Partial Send
	size_t remaining = bytesSent;

	// 1. packets에서 짤린 packet의 iterator 찾기
	auto it = std::find_if(packets.begin(), packets.end(),
		[&remaining](const std::vector<char>& packet)
		{
			if (remaining >= packet.size()) {
				remaining -= packet.size();
				return false;
			}
			return true;
		});

	// 2. Send되지 못한 packet들 다시 SendQueue에 push
	if (it != packets.end() and remaining > 0) {
		std::vector<char> partialPacket(it->begin() + remaining, it->end());
		_sendQueue.push(partialPacket);
		++it;
	}

	for (; it != packets.end(); ++it) {
		_sendQueue.push(*it);
	}

	if (not _sendQueue.empty()) {
		return InternalSend();
	}

	_isSending.store(false);
	if (not _sendQueue.empty() and _isSending.exchange(true) == false) {
		return InternalSend();
	}

	return true;
}

void Session::OnConnect()
{
	LOG_INF("Client Connected : %d", _id);
		
	_isConnected = true;
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

	case CS_ATTACK_END:
		HandleAttackEndPacket(packet);
		break;

	default:
		LOG_WRN("Unknown Packet Type : %d", packetType);
		break;
	}
}

void Session::HandleMovePacket(const std::vector<char>& packet)
{
	auto move = PacketFactory::Deserialize<CS_MOVE_PACKET>(packet);

	if (_character) { 
		LOG_DBG("Session[%d] move : %d / %d", _id, move.direction, move.isRun);					
		_character->SetInput(move.angle, move.direction, move.isRun);
	}
}

void Session::HandleAttackPacket(const std::vector<char>& packet)
{
	auto attack = PacketFactory::Deserialize<CS_ATTACK_PACKET>(packet);

	if (_character) {
		LOG_DBG("Session[%d] attack", _id);
		_character->SetAttackSequence(GetNowTime(), vec3{ attack.x, attack.y, attack.z });
		_service->BroadCast(PacketFactory::SCAttackPacket(*this));
	}
}

void Session::HandleAttackEndPacket(const std::vector<char>& packet)
{
	auto end = PacketFactory::Deserialize<CS_ATTACK_END_PACKET>(packet);

	if (_character) {
		LOG_DBG("Session[%d] attack", _id);
		_service->BroadCast(PacketFactory::SCAttackEndPacket(*this));
	}
}