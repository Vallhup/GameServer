#pragma once

#include <atomic>

namespace disruptor {
	struct alignas(64) AtomicCounter {
		std::atomic<int64_t> value{ 0 };
	};
}