#include "Client.h"

#include <iostream>

#include "../../Graduation Project Server/Graduation Project ServerCore/Protocols/Enum.pb.h"
#include "../../Graduation Project Server/Graduation Project ServerCore/Protocols/Struct.pb.h"
#include "../../Graduation Project Server/Graduation Project ServerCore/Protocols/Protocol.pb.h"

bool Client::Connect()
{
	_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (_socket == INVALID_SOCKET) return false;

	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(7000);
	inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

	if (WSAConnect(_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr),
		nullptr, nullptr, nullptr, nullptr) == SOCKET_ERROR) {
		Disconnect();
		return false;
	}

	RegisterRecv();

	return true;
}

void Client::Disconnect()
{
	std::array<ClientState, 2> expected = { ClientState::ST_ALLOC, ClientState::ST_INGAME };
	for (auto& expect : expected) {
		shutdown(_socket, SD_BOTH);
		CancelIoEx(GetHandle(), nullptr);
		closesocket(_socket);
		_socket = INVALID_SOCKET;
	}
}

void Client::RegisterRecv()
{
	if (_state.load() == ClientState::ST_FREE or _socket == INVALID_SOCKET) {
		Disconnect();
		return;
	}

	DWORD recvFlag{ 0 };
	int wsaBufCount = _recvOver.SetBuffers();
	int result = WSARecv(_socket, _recvOver._wsaBuf.data(), wsaBufCount, NULL, &recvFlag, static_cast<LPWSAOVERLAPPED>(&_recvOver), NULL);
	if (SOCKET_ERROR == result) {
		int error = WSAGetLastError();
		if (WSA_IO_PENDING != error) {
			std::cout << "WSARecv failed : " << error << std::endl;
		}
	}
}

void Client::RegisterSend(const std::vector<char>& packet)
{
	if (_state.load() == ClientState::ST_FREE or _socket == INVALID_SOCKET) {
		Disconnect();
		return;
	}

	_sendQueue.push(packet);

	bool expected{ false };
	if (_isSending.compare_exchange_strong(expected, true)) {
		InternalSend();
	}
}

void Client::Update(float deltaTime)
{
	static constexpr float lerpSpeed{ 10.0f };
	//const float alpha = std::clamp(deltaTime * lerpSpeed, 0.0f, 1.0f);
	const float alpha = 1.0f - expf(-lerpSpeed * deltaTime);
	_pos += (_targetPos - _pos) * alpha;
}

void Client::ProcessRecv(DWORD numBytes)
{
	if (numBytes == 0 or not _recvOver._buffer.Write(nullptr, numBytes)) {
		std::cout << "ProecessRecv Error" << std::endl;
		Disconnect();
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

		ProcessPacket(readBuffer);
	}

	RegisterRecv();
}

void Client::ProcessSend()
{
	InternalSend();
}

HANDLE Client::GetHandle() const
{
	return reinterpret_cast<HANDLE>(_socket);
}

void Client::Dispatch(ExpOver* expOver, int numOfBytes)
{
	if (expOver->CheckOpType(OperationType::Recv)) {
		ProcessRecv(numOfBytes);
	}

	else if (expOver->CheckOpType(OperationType::Send)) {
		ProcessSend();
	}
}

void Client::InternalSend()
{
	if (_state.load() == ClientState::ST_FREE or _socket == INVALID_SOCKET) {
		Disconnect();
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
	int result = WSASend(_socket, sendOver->_wsaBufs.data(), static_cast<DWORD>(sendOver->_wsaBufs.size()), &bytesSent, 0, static_cast<LPWSAOVERLAPPED>(sendOver), NULL);
	if (SOCKET_ERROR == result) {
		int error = WSAGetLastError();
		if ((error == WSAECONNRESET) or (error == WSAENOTCONN) or (error == WSAESHUTDOWN)) {
			std::cout << "WSASend failed : " << error << std::endl;
		}

		_isSending.store(false);
		delete sendOver;

		Disconnect();
		return;
	}
}

void Client::ProcessPacket(const std::vector<char>& packet)
{
	Protocol::GamePacket gamePacket;
	if (not gamePacket.ParseFromArray(packet.data(), packet.size())) {
		return;
	}

	const auto& header = gamePacket.header();
	switch (header.type()) {
	case Protocol::PacketType::SC_LOGIN: {
		Protocol::SC_LOGIN_PACKET login;
		if (not login.ParseFromArray(gamePacket.body().data(), gamePacket.body().size())) {
			return;
		}

		_id = gamePacket.header().sessionid();
		_state.store(ClientState::ST_INGAME);
		break;
	}
	case Protocol::PacketType::SC_MOVE_OBJECT: {
		Protocol::SC_MOVE_PACKET move;
		if (not move.ParseFromArray(gamePacket.body().data(), gamePacket.body().size())) {
			return;
		}

		float x = move.pos().x();
		float y = move.pos().y();
		float z = move.pos().z();
		_targetPos = { x, y, z };
		
		break;
	}
	case Protocol::PacketType::SC_ADD: break;
	case Protocol::PacketType::SC_REMOVE: break;

	case Protocol::PacketType::SC_ATTACK: break;
	case Protocol::PacketType::SC_DODGE: break;
	}
}
