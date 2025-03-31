#include "pch.h"

// 1. 패킷 재조립
// 2. 다중 접속
// 3. 접속 종료

int main()
{
	gServerCore->Init(SERVER_PORT);
	gServerCore->MainLoop();
}
