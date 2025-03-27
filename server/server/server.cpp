#include <iostream>
#include <string>
#include <WS2tcpip.h>

#include "Object.h"

#pragma comment (lib, "WS2_32.LIB")

constexpr short SERVER_PORT = 3000;

Piece chessPiece;

int main()
{
	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 0), &WSAData) != 0)
		return 0;

	SOCKET serverSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, 0);

	SOCKADDR_IN clientAddr;
	clientAddr.sin_family = AF_INET;
	clientAddr.sin_port = htons(SERVER_PORT);
	clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	bind(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), sizeof(SOCKADDR_IN));
	listen(serverSocket, SOMAXCONN);

	INT addrSize = sizeof(SOCKADDR_IN);
	SOCKET clientSocket = WSAAccept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &addrSize, NULL, NULL);

	while (true)
	{
		WPARAM tempParam;
		WPARAM recvParam;

		WSABUF recvWsabuf[1];

		recvWsabuf[0].buf = reinterpret_cast<CHAR*>(&tempParam);
		recvWsabuf[0].len = sizeof(WPARAM);

		DWORD recvBytes;
		DWORD recvFlag = 0;

		WSARecv(clientSocket, recvWsabuf, 1, &recvBytes, &recvFlag, NULL, NULL);
		if (recvBytes == sizeof(WPARAM))
		{
			recvParam = ntohl(tempParam);

			// KeyInput
			chessPiece.KeyInput(recvParam);
		}

		// Send
		WSABUF sendWsabuf[1];
		DWORD sizeSent;

		int32_t networkPos[2];
		size_t totalBytes = sizeof(networkPos);

		networkPos[0] = htonl(chessPiece.xPos);
		networkPos[1] = htonl(chessPiece.yPos);

		sendWsabuf[0].buf = reinterpret_cast<CHAR*>(networkPos);
		sendWsabuf[0].len = static_cast<ULONG>(totalBytes);

		WSASend(clientSocket, sendWsabuf, 1, &sizeSent, 0, NULL, NULL);
	}

	closesocket(clientSocket);
	closesocket(serverSocket);

	WSACleanup();
}
