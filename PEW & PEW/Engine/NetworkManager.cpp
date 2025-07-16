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
	//// 1. read set 초기화, socket Setting
	//fd_set readSet;
	//FD_ZERO(&readSet);
	//FD_SET(clientSocket, &readSet);

	//// 2. timeout 설정, Select
	//timeval timeout{ 0, 0 }; // Non-Blocking Select를 위해 timeout 0으로 설정
	//if (select(0, &readSet, nullptr, nullptr, &timeout) <= 0) {
	//	return;
	//}

	//// 3. Recv 가능한지 판별, 가능하면 Recv 진행
	//if (FD_ISSET(clientSocket, &readSet)) {
	//	char tempBuffer[1024];
	//	int recvLen = recv(clientSocket, tempBuffer, sizeof(tempBuffer), 0);
	//	if (recvLen <= 0) {
	//		Release();
	//		return;
	//	}

	//	if (not recvBuffer.Write(tempBuffer, recvLen)) {
	//		std::cerr << "[RecvBuffer] Write failed or Overflow\n";
	//		return;
	//	}

	//	while (true) {
	//		if (recvBuffer.GetUsedSize() < sizeof(unsigned char)) {
	//			break;
	//		}

	//		unsigned char packetSize{ 0 };
	//		if (not recvBuffer.Peek(reinterpret_cast<char*>(&packetSize), sizeof(unsigned char))) {
	//			break;
	//		}

	//		if (packetSize <= 0 or packetSize > BUFFER_SIZE) {
	//			std::cerr << "Invalid Packet Size : " << packetSize << std::endl;
	//			break;
	//		}

	//		std::vector<char> packet(packetSize);
	//		if (not recvBuffer.Read(packet.data(), packetSize)) {
	//			std::cerr << "RecvBuffer Read Failed\n";
	//			break;
	//		}

	//		ProcessPacket(packet);
	//	}
	//}

	if (not isConnected) {
		return;
	}

	char tempBuffer[1024];
	int recvLen = recv(clientSocket, tempBuffer, sizeof(tempBuffer), 0);
	if (SOCKET_ERROR == recvLen) {
		int error = WSAGetLastError();
		if (WSAEWOULDBLOCK == error) {
			// 읽을 데이터 없음
			return;
		}

		else {
			// 단순 에러 상황
			std::cerr << "recv() error : " << error << std::endl;
			Release();
			return;
		}
	}

	else if (0 >= recvLen) {
		// 연결 종료
		std::cout << "Server DisConneted\n";
		Release();
		return;
	}

	else {
		// 정상 상황 / 패킷 재조립

		// 1. 받은 데이터 RecvBuffer에 추가
		recvBuffer.insert(recvBuffer.end(), tempBuffer, tempBuffer + recvLen);

		// 2. 패킷 재조립
		while (true) {
			// 2-1. 최소 패킷 크기 확인
			//      현재 패킷의 size를 unsigned char로 받고 있음
			if (recvBuffer.size() < sizeof(unsigned char)) {
				break;
			}

			// 2-2. 패킷 size 추출
			unsigned char packetSize = static_cast<unsigned char>(recvBuffer[0]);

			// 2-3. 정상 패킷인지 확인
			//      패킷 사이즈 확인 (현재 Protocol의 최대 사이즈는 23인데 일단 넉넉하게 잡음)
			if (packetSize <= 0 or packetSize > 32) {
				std::cerr << "Invalid Packet Size : " << (int)packetSize << std::endl;
				recvBuffer.clear();
				break;
			}

			// 아직 전체 패킷이 오지 않았을 때
			if (recvBuffer.size() < packetSize) {
				break;
			}

			// 2-4. 정상적인 패킷이 모두 왔을 때
			//      패킷 추출해서 처리
			std::vector<char> packet(recvBuffer.begin(), recvBuffer.begin() + packetSize);
			recvBuffer.erase(recvBuffer.begin(), recvBuffer.begin() + packetSize);

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
			if (addPacket.id < 64) {
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
			else
			{
				int ownerID = addPacket.ownerId;
				Character* character = graphics->GetCharacter(ownerID);
				if (character) {
					glm::vec3 startPos(addPacket.x, addPacket.y, addPacket.z);
					character->CreateBulletFromServer(addPacket.id, startPos);

					std::cout << "[ADD BULLET] ID: " << addPacket.id << " Owner: " << ownerID
						<< " at (" << addPacket.x << ", " << addPacket.y << ", " << addPacket.z << ")" << std::endl;
				}
			}
		}
		break;
	}
	case SC_MOVE_OBJECT:
	{
		SC_MOVE_PACKET movePacket = PacketFactory::Deserialize<SC_MOVE_PACKET>(packet);

		if (graphics) {
			if (movePacket.id < 64) {
				// 캐릭터 이동 처리 (기존 코드)
				Character* character = graphics->GetCharacter(movePacket.id);
				if (character) {
					character->UpdateFromPacket(movePacket.angle, movePacket.x, movePacket.y, movePacket.z, -1, movePacket.isRun);
				}
			}
			else {
				// 총알 이동 처리 - 모든 캐릭터에서 해당 총알 찾기
				glm::vec3 newPos(movePacket.x, movePacket.y, movePacket.z);
				bool bulletFound = false;

				// 모든 캐릭터를 순회하면서 해당 총알 ID 찾기
				for (auto& [id, character] : graphics->GetAllCharacters()) {
					if (character->UpdateBulletFromServer(movePacket.id, newPos)) {
						bulletFound = true;
						break;
					}
				}

				if (bulletFound) {
					std::cout << "[MOVE BULLET] ID: " << movePacket.id
						<< " to (" << movePacket.x << ", " << movePacket.y << ", " << movePacket.z << ")" << std::endl;
				}
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
				character->SetFiring(true);
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