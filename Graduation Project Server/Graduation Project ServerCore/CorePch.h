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
#include <bitset>

#include "Protocols/Enum.pb.h"
#include "Protocols/Struct.pb.h"
#include "Protocols/Protocol.pb.h"

#include "Logger.h"
#include "Macro.h"
#include "vec3.h"
#include "RecvBuffer.h"
#include "ThreadPool.h"
#include "JobQueue.h"
#include "ObjectPool.h"

#include "ExpOver.h"
#include "IocpCore.h"
#include "Listener.h"
#include "Service.h"
#include "Session.h"

#include "CollisionShape.h"
#include "Action.h"
#include "Job.h"
#include "DBJob.h"

#include "SessionManager.h"

#include "IAiBehavior.h"

#include "BTNode.h"
#include "CompositeNode.h"
#include "DecoratorNode.h"
#include "LeafNode.h"

#include "ScriptVM.h"

#include "IComponent.h"
#include "TransformComponent.h"
#include "MovementComponent.h"
#include "InputComponent.h"
#include "ActionComponent.h"
#include "CollisionComponent.h"

#include "GameObject.h"

#include "GameWorld.h"
#include "GameLogic.h"
#include "ObjectManager.h"

#include "Instance.h"
#include "TownInstance.h"
#include "MainInstance.h"
#include "BossInstance.h"
#include "PvpInstance.h"
#include "TestInstance.h"

#include "UserRepository.h"
#include "DBManager.h"

#include "PacketFactory.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "MSWSock.LIB")
