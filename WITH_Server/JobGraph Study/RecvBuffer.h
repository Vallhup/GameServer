#pragma once

#include <vector>
#include "Constants.h"

class RecvBuffer {
public:
	RecvBuffer(int bufferSize = BUFFER_SIZE);

public:
	char* GetBuffer() { return _buffer.data(); }
	char* GetReadPos() { return &_buffer[_readPos]; }
	char* GetWritePos() { return &_buffer[_writePos]; }
	int   GetUsedSize() const;
	int   GetFreeSize() const;
	int   GetContiguousUsedSize() const;
	int	  GetContiguousFreeSize() const;

public:
	bool Read(char* readBuffer, int readSize);
	bool Write(const char* data, int dataSize);

	bool Peek(void* outBuffer, int size) const;

private:
	std::vector<char> _buffer;
	int		_readPos{ 0 };
	int		_writePos{ 0 };
};