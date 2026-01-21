#pragma once

#include <vector>
#include "types.h"

constexpr short BUFFER_SIZE = 4096;

class RecvBuffer
{
public:
	RecvBuffer(int bufferSize = BUFFER_SIZE);
	~RecvBuffer() = default;

public:
	BYTE* GetBuffer() { return _buffer.data(); }
	BYTE* GetWritePos() { return &_buffer[_writePos]; }
	int   GetUsedSize() const;
	int   GetFreeSize() const;
	int	  GetContiguousFreeSize() const;

public:
	bool Read(BYTE* readBuffer, int readSize);
	bool Write(const BYTE* data, int dataSize);

	bool Peek(void* outBuffer, int size) const;

private:
	std::vector<BYTE> _buffer;
	int		_readPos{ 0 };
	int		_writePos{ 0 };
};