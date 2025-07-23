#pragma once

#define NOMINMAX

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
#include <concurrent_queue.h>
#include <concurrent_vector.h>
#include <concurrent_priority_queue.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <algorithm>
#include <type_traits>
#include <chrono>
#include <random>
#include <functional>
#include <codecvt>
#include <future>
#include <numeric>

#include "Service.h"
#include "Listener.h"
#include "Session.h"


#include "Character.h"
#include "Projectile.h"

#include "CharacterManager.h"
#include "ProjectileManager.h"
#include "CollisionManager.h"
#include "SessionManager.h"
#include "TimerManager.h"
#include "GameLogic.h"

#include "RecvBuffer.h"

#include "Macro.h"
#include "Logger.h"

#include "PacketFactory.h"
#include "protocol.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "MSWSock.LIB")

constexpr float PI = 3.141592f;