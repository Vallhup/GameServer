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
#include <concurrent_unordered_map.h>
#include <concurrent_queue.h>
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
#include <functional>
#include <codecvt>
#include <future>
#include <ranges>

#include "Logger.h"
#include "Macro.h"
#include "Math.h"
#include "RecvBuffer.h"
#include "CollisionShape.h"

#include "ExpOver.h"
#include "IocpCore.h"
#include "Listener.h"
#include "Service.h"
#include "Session.h"

#include "SessionManager.h"
#include "EventManager.h"

#include "TickScheduler.h"

#include "Input.h"

#include "IAiBehavior.h"

#include "IComponent.h"
#include "TransformComponent.h"
#include "MovementComponent.h"
#include "InputComponent.h"

#include "GameObject.h"
#include "Character.h"
#include "Monster.h"

#include "GameWorld.h"
#include "GameLogic.h"
#include "ObjectManager.h"

#include "Instance.h"
#include "TownInstance.h"
#include "MainInstance.h"
#include "BossInstance.h"
#include "PvpInstance.h"


#include "UserRepository.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "MSWSock.LIB")
#pragma comment(lib, "lua54.lib")
