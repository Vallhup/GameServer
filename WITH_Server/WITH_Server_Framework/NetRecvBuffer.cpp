#include "pch.h"
#include "NetRecvBuffer.h"
#include "Protocol.h"

NetRecvBuffer::NetRecvBuffer(uint32_t capacity)
{
	// Power-of-2 enforcement for bitwise AND indexing
	uint32_t realCapacity = 1;
	while (realCapacity < capacity)
		realCapacity <<= 1;

	_buffer.resize(realCapacity);
	_mask = realCapacity - 1;
}

int NetRecvBuffer::PrepareWsaBufs(WSABUF bufs[2]) noexcept
{
	const uint32_t used      = _writePos - _readPos;
	const uint32_t capacity  = _mask + 1;
	const uint32_t freeSpace = capacity - used;

	if (freeSpace == 0) return 0;

	const uint32_t writeIdx = _writePos & _mask;
	const uint32_t toEnd    = capacity - writeIdx;

	bufs[0].buf = reinterpret_cast<char*>(_buffer.data() + writeIdx);

	if (freeSpace <= toEnd)
	{
		bufs[0].len = freeSpace;
		return 1;
	}

	bufs[0].len    = toEnd;
	bufs[1].buf    = reinterpret_cast<char*>(_buffer.data());
	bufs[1].len    = freeSpace - toEnd;
	return 2;
}

bool NetRecvBuffer::CommitWrite(uint32_t bytes) noexcept
{
	const uint32_t freeSpace = (_mask + 1) - (_writePos - _readPos);
	if (bytes > freeSpace) return false;
	_writePos += bytes;
	return true;
}

std::span<const std::byte> NetRecvBuffer::PeekPacket() const noexcept
{
	const uint32_t available  = _writePos - _readPos;
	constexpr uint32_t kHeaderSize = static_cast<uint32_t>(sizeof(PacketHeader));

	if (available < kHeaderSize) return {};

	const uint32_t readIdx = _readPos & _mask;
	const uint32_t toEnd   = (_mask + 1) - readIdx;

	// Read header — may cross ring boundary
	PacketHeader header{};
	if (toEnd >= kHeaderSize)
	{
		std::memcpy(&header, _buffer.data() + readIdx, kHeaderSize);
	}
	else
	{
		// Header straddles the ring wrap-around
		std::byte tmp[kHeaderSize];
		std::memcpy(tmp, _buffer.data() + readIdx, toEnd);
		std::memcpy(tmp + toEnd, _buffer.data(), kHeaderSize - toEnd);
		std::memcpy(&header, tmp, kHeaderSize);
	}

	const uint32_t packetSize = header.size;
	if (packetSize < kHeaderSize || packetSize > (_mask + 1)) return {};
	if (available < packetSize) return {};

	// Return zero-copy span when packet lies in a contiguous region
	if (toEnd >= packetSize)
		return std::span<const std::byte>(_buffer.data() + readIdx, packetSize);

	// Packet straddles the boundary — linearize into scratch buffer
	_linearBuf.resize(packetSize);
	std::memcpy(_linearBuf.data(), _buffer.data() + readIdx, toEnd);
	std::memcpy(_linearBuf.data() + toEnd, _buffer.data(), packetSize - toEnd);
	return std::span<const std::byte>(_linearBuf.data(), packetSize);
}

void NetRecvBuffer::ConsumePacket(uint32_t packetSize) noexcept
{
	_readPos += packetSize;
}
