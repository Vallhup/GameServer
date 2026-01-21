#pragma once

#include <iostream>
#include <vector>
#include <array>
#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <numeric>
#include <ranges>

#include <concurrent_queue.h>

#include "asio.hpp"

#include "Protocol.pb.h"

#include "Asio_Network_Library.h"
#include "ProtocolLib.h"

#pragma comment(lib, "Asio_Network_Library.lib")