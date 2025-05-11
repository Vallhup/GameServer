#pragma once

#include <winsock2.h>
#include <mswsock.h>
#include <WS2tcpip.h>
#include <Windows.h>

#include <iostream>
#include <vector>
#include <array>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <concurrent_unordered_map.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <algorithm>

#include "RecvBuffer.h"
#include "AtomicQueue.h"
#include "Session.h"
#include "Packet.h"
#include "ExpOver.h"
#include "Listener.h"
#include "IocpCore.h"
#include "Service.h"
#include "AStar.h"
#include "Logger.h"
#include "Macro.h"


#pragma comment(lib, "ws2_32.lib")
#pragma comment (lib, "MSWSock.LIB")

constexpr short SERVER_PORT = 3000;

using ServicePtr = std::shared_ptr<Service>;
using IocpCorePtr = std::shared_ptr<IocpCore>;
using SessionPtr= std::shared_ptr<Session>;
using ListenerPtr = std::shared_ptr<Listener>;
