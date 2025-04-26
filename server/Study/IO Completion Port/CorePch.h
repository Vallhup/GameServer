#pragma once

#include <winsock2.h>
#include <mswsock.h>
#include <WS2tcpip.h>
#include <Windows.h>

#include <iostream>
#include <vector>
#include <array>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <memory>

#include "RecvBuffer.h"
#include "AtomicQueue.h"
#include "Session.h"
#include "Packet.h"
#include "ExpOver.h"
#include "Listener.h"
#include "IocpCore.h"
#include "Service.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment (lib, "MSWSock.LIB")

constexpr short SERVER_PORT = 3000;