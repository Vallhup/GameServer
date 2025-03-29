#include "ServerCore.h"

#pragma comment (lib, "WS2_32.LIB")

// 1. Connect Packet, DisConnect Packet
// 2. Packet贸府 备泅 
// 3. Object Class 沥府
// 4. client, server main loop 沥府

int main()
{
	gServerCore->MainLoop();

	//while (true)
	//{
	//	WPARAM tempParam;
	//	WPARAM recvParam;

	//	WSABUF recvWsabuf[1];

	//	recvWsabuf[0].buf = reinterpret_cast<CHAR*>(&tempParam);
	//	recvWsabuf[0].len = sizeof(WPARAM);

	//	DWORD recvBytes;
	//	DWORD recvFlag = 0;

	//	WSARecv(clientSocket, recvWsabuf, 1, &recvBytes, &recvFlag, NULL, NULL);
	//	if (recvBytes == sizeof(WPARAM))
	//	{
	//		recvParam = ntohl(tempParam);

	//		// KeyInput
	//		chessPiece.KeyInput(recvParam);
	//	}

	//	// Send
	//	WSABUF sendWsabuf[1];
	//	DWORD sizeSent;

	//	int32_t networkPos[2];
	//	size_t totalBytes = sizeof(networkPos);

	//	networkPos[0] = htonl(chessPiece.xPos);
	//	networkPos[1] = htonl(chessPiece.yPos);

	//	sendWsabuf[0].buf = reinterpret_cast<CHAR*>(networkPos);
	//	sendWsabuf[0].len = static_cast<ULONG>(totalBytes);

	//	WSASend(clientSocket, sendWsabuf, 1, &sizeSent, 0, NULL, NULL);
	//}

	//closesocket(clientSocket);
	//closesocket(serverSocket);

	//WSACleanup();
}
