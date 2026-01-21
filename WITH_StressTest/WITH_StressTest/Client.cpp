#include "pch.h"
#include "Client.h"
#include "SendBuffer.h"

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

void Client::RegisterSend(SendBuffer* data)
{
	if (!data) return;
	if (_state.load() == ClientState::ST_FREE or _socket == INVALID_SOCKET) {
		SendBufferPool::Get().Release(data);
		Disconnect();
		return;
	}

	_sendQueue.push(data);

	bool expected{ false };
	if (_isSending.compare_exchange_strong(expected, true)) {
		InternalSend();
	}
}

void Client::Update(float deltaTime)
{
	static constexpr float lerpSpeed{ 10.0f };
	const float alpha = 1.0f - expf(-lerpSpeed * deltaTime);
	_pos += (_targetPos - _pos) * alpha;
}

void Client::ProcessRecv(DWORD numBytes)
{
	if (numBytes == 0 || !_recvOver._buffer.Write(nullptr, numBytes)) {
		Disconnect();
		return;
	}

	while (true) {
		// 1) 헤더 최소 크기 확인
		if (_recvOver._buffer.GetUsedSize() < sizeof(PacketHeader))
			break;

		// 2) 헤더 Peek
		PacketHeader header{};
		if (!_recvOver._buffer.Peek(&header, sizeof(PacketHeader)))
			break;

		// 3) 헤더 검증
		if (header.size < sizeof(PacketHeader)) {
			Disconnect();
			return;
		}

		// 4) 패킷 전체가 도착했는지 확인
		if (_recvOver._buffer.GetUsedSize() < header.size)
			break;

		// 5) 패킷 전체를 임시 버퍼로 Read
		//    (수신은 풀을 쓸 이유가 약하므로 std::vector로 충분합니다)
		std::vector<BYTE> packet(header.size);
		_recvOver._buffer.Read(packet.data(), header.size);

		// 6) 처리
		ProcessPacket(reinterpret_cast<const PacketHeader&>(*packet.data()),
			packet.data());
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
		auto* sendOver = static_cast<SendOver*>(expOver);

		for (SendBuffer* buffer : sendOver->_buffers)
			SendBufferPool::Get().Release(buffer);

		delete sendOver;
		ProcessSend();
	}
}

void Client::InternalSend()
{
	if (_state.load() == ClientState::ST_FREE or _socket == INVALID_SOCKET) {
		Disconnect();
		return;
	}
	
	std::vector<SendBuffer*> packets;
	packets.reserve(MAX_PACKET);

	SendBuffer* sendData;
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

		for (SendBuffer* buffer : sendOver->_buffers)
			SendBufferPool::Get().Release(buffer);

		delete sendOver;

		Disconnect();
		return;
	}
}

void Client::ProcessPacket(const PacketHeader& header, const BYTE* data)
{
	PacketType type = static_cast<PacketType>(header.type);

	switch (type) {
	case PacketType::SC_LOGIN:
	{
		Protocol::SC_LOGIN_PACKET login;
		if (PacketFactory::Deserialize<Protocol::SC_LOGIN_PACKET>(
			header, data, &login))
		{
			_id = login.sessionid();
			_state.store(ClientState::ST_INGAME);
		}
		break;
	}
	case PacketType::SC_MOVE_OBJECT:
	{
		Protocol::SC_MOVE_PACKET move;
		if (PacketFactory::Deserialize<Protocol::SC_MOVE_PACKET>(
			header, data, &move))
		{
			float x = move.x();
			float y = move.y();
			float z = move.z();

			_targetPos = { x, y, z };
			break;
		}
	}
	}
}
