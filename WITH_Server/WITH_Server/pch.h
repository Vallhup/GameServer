#pragma once

#include <fstream>
#include <iostream>
#include <vector>
#include <memory>
#include <unordered_map>
#include <DirectXMath.h>

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <stack>
#include <array>
#include <deque>
#include <concurrent_queue.h>
#include <typeindex>

#include "asio.hpp"

#include "Protocol.pb.h"

#include "Asio_Network_Library.h"
#include "ProtocolLib.h"

#pragma comment(lib, "Asio_Network_Library.lib")

#include "ECS.h"
#include "ActionManager.h"
#include "AnimationManager.h"
#include "DBManager.h"
#include "MapCollisionManager.h"

#include "NetHelper.h"
#include "Collision.h"
#include "Math.h"