#pragma once

#include <atomic>
#include <memory>
#include <concurrent_queue.h>

struct SendBuffer {
	uint32 size;
	uint32 capacity;
	BYTE* data{ nullptr };
	std::atomic<uint32_t> refCount{ 1 };

	void AddRef() noexcept
	{
		refCount.fetch_add(1, std::memory_order_acq_rel);
	}

	bool TryAppend(const void* src, uint32 len);
	void Reset();
};

class SendBufferPool {
	static constexpr std::array<uint32, 8> kCapacityList
	{ 1024, 4096, 16384, 65536, 131072, 262144, 524288, 1048576 };

	static constexpr uint64 kCacheAlign{ 64 };
	static constexpr uint32 kInvalidLevel
	{ (std::numeric_limits<uint32>::max)() };

public:
	static SendBufferPool& Get()
	{
		static SendBufferPool instance;
		return instance;
	}

	SendBuffer* Acquire(uint32 capacity);
	void Release(SendBuffer* buf);

private:
	uint32 PickLevel(uint32 capacity);
	uint32 LevelIndexByCapacity(uint32 capacity);
	SendBuffer* AllocateNew(uint32 capacity);
	void Destroy(SendBuffer* buf);

	std::array<concurrency::concurrent_queue<SendBuffer*>,
		kCapacityList.size()> _free;
};

struct SendBufferDeleter {
	void operator()(SendBuffer* p) const noexcept
	{
		if (p) SendBufferPool::Get().Release(p);
	}
};

using SendBufferPtr = std::unique_ptr<SendBuffer, SendBufferDeleter>;