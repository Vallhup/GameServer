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
#include <concurrent_priority_queue.h>
#include <concurrent_unordered_set.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <algorithm>
#include <type_traits>
#include <chrono>
#include <random>

#pragma comment(lib, "ws2_32.lib")
#pragma comment (lib, "MSWSock.LIB")

#include "RecvBuffer.h"
#include "AtomicQueue.h"

#include "ExpOver.h"
#include "IocpCore.h"

#include "Service.h"
#include "Session.h"
#include "Listener.h"
#include "Sector.h"
#include "GameObject.h"

#include "AStar.h"
#include "ChatManager.h"
#include "ViewListHelper.h"
#include "PacketFactory.h"

#include "Logger.h"
#include "Macro.h"
#include "protocol.h"

constexpr short SERVER_PORT = 4000;

using ServicePtr = std::shared_ptr<class Service>;
using IocpCorePtr = std::shared_ptr<class IocpCore>;
using GameObjectPtr = std::shared_ptr<class GameObject>;
using SessionPtr = std::shared_ptr<class Session>;
using GameSessionPtr = std::shared_ptr<class GameSession>;
using ListenerPtr = std::shared_ptr<class Listener>;