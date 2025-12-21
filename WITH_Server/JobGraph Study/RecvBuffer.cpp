#include "RecvBuffer.h"

RecvBuffer::RecvBuffer(int bufferSize) : _readPos(0), _writePos(0)
{
	_buffer.resize(bufferSize);
}

int RecvBuffer::GetUsedSize() const
{
	if (_writePos >= _readPos)
		return _writePos - _readPos;

	else
		return static_cast<int>(_buffer.size()) - _readPos + _writePos;
}

int RecvBuffer::GetFreeSize() const
{
	return static_cast<int>(_buffer.size()) - GetUsedSize() - 1;
}

int RecvBuffer::GetContiguousUsedSize() const
{
	if (_writePos >= _readPos)
		return _writePos - _readPos;

	else
		return static_cast<int>(_buffer.size()) - _readPos;
}

int RecvBuffer::GetContiguousFreeSize() const
{
	if (_writePos >= _readPos)
		return static_cast<int>(_buffer.size()) - _writePos;

	else 
		return _readPos - _writePos - 1;
}

bool RecvBuffer::Write(const char* data, int dataSize)
{
	if (dataSize <= 0) return false;
	if (dataSize > GetFreeSize()) return false;

	if (data != nullptr) 
	{
		int	sizeToEnd = static_cast<int>(_buffer.size()) - _writePos;
		int firstCopySize = std::min<int>(dataSize, sizeToEnd);
		int secondCopySize = dataSize - firstCopySize;

		std::memcpy(&_buffer[_writePos], data, firstCopySize);
		std::memcpy(&_buffer[0], data + firstCopySize, secondCopySize);
	}

	_writePos = (_writePos + dataSize) % static_cast<int>(_buffer.size());

	return true;
}

bool RecvBuffer::Peek(void* outBuffer, int size) const
{
	if (size <= 0) return false;
	if (size > GetUsedSize()) return false;

	int	sizeToEnd = static_cast<int>(_buffer.size()) - _readPos;
	int firstCopySize = std::min<int>(size, sizeToEnd);
	int secondCopySize = size - firstCopySize;

	std::memcpy(outBuffer, &_buffer[_readPos], firstCopySize);
	std::memcpy(static_cast<char*>(outBuffer) + firstCopySize, &_buffer[0], secondCopySize);

	return true;
}

bool RecvBuffer::Read(char* readBuffer, int readSize)
{
	if (readSize <= 0) return false;
	if (readSize > GetUsedSize()) return false;

	if (readBuffer != nullptr) 
	{
		int sizeToEnd = static_cast<int>(_buffer.size()) - _readPos;
		int firstCopySize = std::min<int>(readSize, sizeToEnd);
		int secondCopySize = readSize - firstCopySize;

		std::memcpy(readBuffer, &_buffer[_readPos], firstCopySize);
		std::memcpy(readBuffer + firstCopySize, &_buffer[0], secondCopySize);
	}

	_readPos = (_readPos + readSize) % static_cast<int>(_buffer.size());

	return true;
}

