#pragma once

struct PacketHeader;

class NetRecvBuffer {
	static constexpr uint32_t kDefaultCapacity{ 65536 };

public:
	explicit NetRecvBuffer(uint32_t capacity = kDefaultCapacity);

	int  PrepareWsaBufs(WSABUF bufs[2]) noexcept;
	bool CommitWrite(uint32_t bytes) noexcept;

	uint32_t AvailableBytes() const noexcept;
	bool TryPeekHeader(PacketHeader& outHeader) const noexcept;
	std::span<const std::byte> PeekPacket() const noexcept;
	void ConsumePacket(uint32_t packetSize) noexcept;

private:
	std::vector<std::byte>			_buffer;
	uint32_t						_mask;
	uint32_t						_readPos{ 0 };
	uint32_t						_writePos{ 0 };
	mutable std::vector<std::byte>	_linearBuf;
};

