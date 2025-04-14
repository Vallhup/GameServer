#pragma once

// 1. Ring Buffer
// 2. char* 사용

// 4KB
constexpr short BUFFER_SIZE = 4096;

// RecvOver에 들어갈 Buffer
class RecvBuffer
{
public:
	RecvBuffer(int bufferSize = BUFFER_SIZE);

	~RecvBuffer() { delete[] _buffer; }

public:
	// Getter
	char* GetBuffer()   const { return _buffer; }
	char* GetWritePos() const { return &_buffer[_writePos]; }
	
	// 빈 공간이 있는지 어떻게 확인해야 하는가
	// -> 전체 Size (_capacity)에서 현재 사용중인 Size를 빼면 됨
	// 
	// 현재 사용중인 Size 구하기
	// 1. writePos > readPos
	//  (writePos - readPos)
	// 
	// 2. writePos < readPos
	//  (capacity - readPos + writePos)
	// 
	// 3. writePos = readPos
	//  0 = (writePos - readPos)

	int   GetUsedSize() const { return (_writePos >= _readPos) ? (_writePos - _readPos) : (_capacity - _readPos + _writePos); }
	// -1을 하는 이유? _writePos와 _readPos가 같은 값이 되면 빈 버퍼와 구분할 수 없기 때문
	int   GetFreeSize() const { return _capacity - GetUsedSize() - 1; }

public:
	// 외부에서 사용
	bool Write(const char* data, int dataSize);
	bool Read(char* readBuffer, int readSize);

private:
	// 실제 Buffer
	char*	_buffer;

	// Buffer가 할당한 메모리 Size
	int		_capacity{ 0 };

	int		_readPos{ 0 };
	int		_writePos{ 0 };
};

