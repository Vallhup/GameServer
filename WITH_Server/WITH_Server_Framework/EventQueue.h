#pragma once

#include <span>
#include <vector>
#include <type_traits>
#include <utility>

template<typename T, template<typename> class BufferPolicy>
class EventQueue {
public:
	void Publish(const T& event) { _write.push(event); }
	void Publish(T&& event) { _write.push(std::move(event)); }

	void SwapBuffers()
	{
		_read.clear();
		_read.swap(_write);
	}

	[[nodiscard]] std::span<const T> ConsumeView() const { return _read.view(); }

	void ClearAll() { _read.clear(); _write.clear(); }

private:
	BufferPolicy<T> _read;
	BufferPolicy<T> _write;
};

template<typename T>
class SingleThreadBuffer {
	static_assert(std::is_move_constructible_v<T>, "T must be move constructible");

public:
	void push(const T& item) { _buffer.push_back(item); }
	void push(T&& item) { _buffer.push_back(std::move(item)); }

	void clear() { _buffer.clear(); }

	std::span<const T> view() const { return _buffer; }

	void swap(SingleThreadBuffer& other) noexcept
	{
		_buffer.swap(other._buffer);
	}

private:
	std::vector<T> _buffer;
};

