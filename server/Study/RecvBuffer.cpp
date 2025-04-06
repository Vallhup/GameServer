#include "pch.h"
#include "RecvBuffer.h"

RecvBuffer::RecvBuffer(int bufferSize) : _size(0), _readPos(0), _writePos(0), _capacity(bufferSize)
{
	_buffer = new char[bufferSize];
}

bool RecvBuffer::Write(const char* data, int dataSize)
{
	// 1. Buffer에 빈 공간이 있는지 확인
	//  - 있다면 Write 진행
	//  - 없다면 false를 return하고 나중에 Write진행
	
	if (dataSize > GetFreeSize()) {
		return false;
	}

	// 2. Write 진행
	//  - data를 _buffer에 복사하고
	//  - writePos를 dataSize만큼 이동

	// 유의할 점
	// dataSize가 _capacity - _writePos 보다 클 때
	// _capacity - _writePos만큼만 복사하고
	// 남은 Data를 _buffer의 첫 부분에 복사해야 함

	int	sizeToEnd = _capacity - _writePos;
	int firstCopySize = std::min(dataSize, sizeToEnd);
	int secondCopySize = dataSize - firstCopySize;

	std::memcpy(_buffer + _writePos, data, firstCopySize);
	std::memcpy(_buffer, data + firstCopySize, secondCopySize);

	_writePos = (_writePos + dataSize) % _capacity;

	return true;
}

bool RecvBuffer::Read(char* readBuffer, int readSize)
{
	// 1. 읽을 Size만큼 Data가 들어가 있는지 확인
	//  - 있다면 Read 진행
	//  - 없다면 false를 return하고 나중에 Read 진행

	if (readSize > GetUsedSize()) {
		return false;
	}

	// 2. Read 진행
	//  - data를 readBuffer에 복사하고
	//  - readPos를 readSize만큼 이동

	// 유의할 점
	// dataSize가 _capacity - _readPos 보다 클 때
	// _capacity - _readPos만큼만 복사하고
	// 남은 Size만큼 _buffer의 첫 부분에서 복사해야 됨
	
	int sizeToEnd = _capacity - _readPos;
	int firstCopySize = std::min(readSize, sizeToEnd);
	int secondCopySize = readSize - firstCopySize;

	std::memcpy(readBuffer, _buffer + _readPos, firstCopySize);
	std::memcpy(readBuffer + firstCopySize, _buffer, secondCopySize);

	_readPos = (_readPos + readSize) % _capacity;

	return true;
}

