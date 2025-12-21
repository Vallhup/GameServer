#pragma once

#include <atomic>
#include <array>
#include <vector>

template<typename T>
class TripleBuffer {
public:
	TripleBuffer() : _write(0), _read(1), _middle(2) {}

	T& WriteBuffer()
	{
		return _buffers[_write];
	}

	void Publish()
	{
		int write = _write;
		int middle = _middle.exchange(write);
		_write = middle;
	}

	void SwapIfUpdated()
	{
		int middle = _middle;
		if (middle != _read)
		{
			int read = _read.exchange(middle);
			_middle = read;
		}
	}

	const T& ReadBuffer()
	{
		return _buffers[_read];
	}

private:
	std::array<T, 3> _buffers;
	std::atomic<int> _write;
	std::atomic<int> _read;
	std::atomic<int> _middle;

};

