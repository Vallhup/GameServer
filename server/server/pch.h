#pragma once

#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.

#include <iostream>
#include <Windows.h>
#include <unordered_map>
#include <algorithm>

#include "ExpOver.h"
#include "Packet.h"
#include "ServerCore.h"
#include "Session.h"

#include <Winsock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32.lib")