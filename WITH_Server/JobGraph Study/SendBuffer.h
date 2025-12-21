#pragma once

#include <vector>
#include "Constants.h"

class SendBuffer {
public:
	SendBuffer(int size = BUFFER_SIZE);

	SendBuffer(const SendBuffer& other) 
		: _buffer(other._buffer), _writePos(other._writePos) {}
	SendBuffer& operator=(const SendBuffer& other)
	{
		_buffer = other._buffer;
		_writePos = other._writePos;
		return *this;
	}

	void Write(const void* data, int size);

	const char* data() const { return _buffer.data(); }
	char* data() { return _buffer.data(); }
	int   size() const { return _writePos; }
	void  clear() { _writePos = 0; }

private:
	std::vector<char> _buffer;
	int _writePos;
};

