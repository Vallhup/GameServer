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
#include "WITH_Server_Framework.h"

#pragma comment(lib, "Asio_Network_Library.lib")
#pragma comment(lib, "WITH_Server_Framework.lib")

#include "DBManager.h"

#include "NetHelper.h"
#include "Collision.h"
#include "Math.h"