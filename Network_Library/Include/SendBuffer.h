#pragma once

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