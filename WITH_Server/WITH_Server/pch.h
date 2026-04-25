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

#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 2451 4244 4251 4267 4996)
#elif defined(__GNUC__) || defined(__clang__)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-parameter"
	#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "Protocol.pb.h"

#ifdef _MSC_VER
	#pragma warning(pop)
#elif defined(__GNUC__) || defined(__clang__)
	#pragma GCC diagnostic pop
#endif

#include "Asio_Network_Library.h"
#include "ProtocolLib.h"
#include "WITH_Server_Framework.h"

#pragma comment(lib, "Asio_Network_Library.lib")
#pragma comment(lib, "WITH_Server_Framework.lib")
#pragma comment(lib, "Detour.lib")
#pragma comment(lib, "Recast.lib")

#include "DBManager.h"