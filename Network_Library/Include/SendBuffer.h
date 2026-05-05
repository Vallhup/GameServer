#pragma once

#include <memory>
#include <concurrent_queue.h>

struct SendBuffer {
	uint32 size;
	uint32 capacity;
	BYTE* data{ nullptr };

	SendBuffer* Clone() const;
	bool TryAppend(const void* src, uint32 len);
	void Reset();
};

class SendBufferPool {
	static constexpr std::array<uint32, 4> kCapacityList
	{ 1024, 4096, 16384, 65536 };

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

// SendBuffer RAII 소유권 래퍼.
// 소멸 시 SendBufferPool::Get().Release() 를 자동 호출한다.
// payloadKey 로 소유권을 이전할 때는 release() 를 사용하고,
// 재획득 시에는 SendBufferPtr(raw_ptr) 로 래핑한다.
struct SendBufferDeleter
{
    void operator()(SendBuffer* buf) const noexcept
    {
        if (buf)
            SendBufferPool::Get().Release(buf);
    }
};
using SendBufferPtr = std::unique_ptr<SendBuffer, SendBufferDeleter>;