#include "pch.h"
#include "NetworkManager.h"
#include "PacketFactory.h"
#include "GraphicsManager.h"
#include "Character.h"

NetworkManager::NetworkManager()
{
	clientSocket = INVALID_SOCKET;
	isConnected = false;
}

NetworkManager::~NetworkManager()
{
	if (isConnected)
		Release();
}

void NetworkManager::Init(const char* IP, u_short port)
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cout << "WSAStartup failed!" << endl;
		return;
	}

	clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET)
	{
		cout << "Socket creation failed!" << endl;
		WSACleanup();
		return;
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	inet_pton(AF_INET, IP, &serverAddr.sin_addr);

	if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		cout << "Connection failed!" << endl;
		closesocket(clientSocket);
		WSACleanup();
		return;
	}

	// Socket을 Non-Blocking으로 변경
	u_long nonBlocking{ 1 };
	ioctlsocket(clientSocket, FIONBIO, &nonBlocking);

	isConnected = true;
	cout << "Connected to Server!" << endl;
}

void NetworkManager::Update()
{
	// 1. read set 초기화, socket Setting
	fd_set readSet;
	FD_ZERO(&readSet);
	FD_SET(clientSocket, &readSet);

	// 2. timeout 설정, Select
	timeval timeout{ 0, 0 }; // Non-Blocking Select를 위해 timeout 0으로 설정
	if (select(0, &readSet, nullptr, nullptr, &timeout) <= 0) {
		return;
	}

	// 3. Recv 가능한지 판별, 가능하면 Recv 진행
	if (FD_ISSET(clientSocket, &readSet)) {
		char tempBuffer[1024];
		int recvLen = recv(clientSocket, tempBuffer, sizeof(tempBuffer), 0);
		if (recvLen <= 0) {
			Release();
			return;
		}

		if (not recvBuffer.Write(tempBuffer, recvLen)) {
			std::cerr << "[RecvBuffer] Write failed or Overflow\n";
			return;
		}

		while (true) {
			if (recvBuffer.GetUsedSize() < sizeof(unsigned char)) {
				break;
			}

			unsigned char packetSize{ 0 };
			if (not recvBuffer.Peek(reinterpret_cast<char*>(&packetSize), sizeof(unsigned char))) {
				break;
			}

			if (packetSize <= 0 or packetSize > BUFFER_SIZE) {
				std::cerr << "Invalid Packet Size : " << packetSize << std::endl;
				break;
			}

			std::vector<char> packet(packetSize);
			if (not recvBuffer.Read(packet.data(), packetSize)) {
				std::cerr << "RecvBuffer Read Failed\n";
				break;
			}

			ProcessPacket(packet);
		}
	}
}

void NetworkManager::Release()
{
	if (isConnected)
	{
		closesocket(clientSocket);
		isConnected = false;

		cout << "Socket closed!" << endl;
	}
	WSACleanup();
}

void NetworkManager::Send(const std::vector<char>& packet)
{
	if (SOCKET_ERROR == send(clientSocket, packet.data(), packet.size(), 0)) {
		int error = WSAGetLastError();
		if (WSAEWOULDBLOCK == error) {
			// 송신 버퍼 OverFlow -> 별도 처리 X (패킷 누락)
		}
		else {
			Release();
		}
	}
}

void NetworkManager::ProcessPacket(const std::vector<char>& packet)
{
	if (packet.size() < 2) return;

	char packetType = packet[1];

	switch (packetType) {
	case SC_ADD:
	{
		SC_ADD_PACKET addPacket = PacketFactory::Deserialize<SC_ADD_PACKET>(packet);

		if (graphics) {
			// 첫 번째 받은 캐릭터를 내 캐릭터로 설정
			static bool firstCharacter = true;
			bool isLocal = firstCharacter;
			firstCharacter = false;

			graphics->AddCharacter(addPacket.id, isLocal);

			Character* character = graphics->GetCharacter(addPacket.id);
			if (character) {
				character->SetTargetPosition(addPacket.x, addPacket.y, addPacket.z);
			}

			std::cout << "[ADD PLAYER] ID: " << addPacket.id << " at ("
				<< addPacket.x << ", " << addPacket.y << ", " << addPacket.z << ")"
				<< (isLocal ? " (LOCAL)" : " (REMOTE)") << std::endl;
		}
		break;
	}
	case SC_MOVE_OBJECT:
	{
		SC_MOVE_PACKET movePacket = PacketFactory::Deserialize<SC_MOVE_PACKET>(packet);

		if (graphics) {
			Character* character = graphics->GetCharacter(movePacket.id);
			if (character) {
				character->UpdateFromPacket(movePacket.x, movePacket.y, movePacket.z);
			}
		}
		break;
	}
	case SC_REMOVE:
	{
		SC_REMOVE_PACKET removePacket = PacketFactory::Deserialize<SC_REMOVE_PACKET>(packet);

		if (graphics) {
			graphics->RemoveCharacter(removePacket.id);
		}

		std::cout << "[REMOVE PLAYER] ID: " << removePacket.id << std::endl;
		break;
	}
	case SC_ATTACK:
	{
		SC_ATTACK_PACKET attackPacket = PacketFactory::Deserialize<SC_ATTACK_PACKET>(packet);

		if (graphics) {
			Character* character = graphics->GetCharacter(attackPacket.id);
			if (character) {
				// 공격 애니메이션 처리 (나중에 구현)
				std::cout << "[ATTACK] Player ID: " << attackPacket.id << std::endl;
			}
		}
		break;
	}
	default:
		std::cout << "[UNKNOWN PACKET] Type: " << (int)packetType << std::endl;
		break;
	}
}

bool NetworkManager::IsConnected() const
{
	return isConnected;
}