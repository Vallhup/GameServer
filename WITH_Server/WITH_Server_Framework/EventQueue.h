#pragma once

#include <span>
#include <vector>
#include <type_traits>
#include <utility>

template<typename T, template<typename> class BufferPolicy>
class EventQueue {
public:
	void Publish(const T& event) { _buffer.push(event); }
	void Publish(T&& event) { _buffer.push(std::move(event)); }

	/*void SwapBuffers()
	{
		_read.clear();
		_read.swap(_write);
	}*/

	[[nodiscard]] std::span<T> ConsumeView() { return _buffer.view(); }

	size_t Size() const { return _buffer.size(); }
	void ReadResize(size_t n) { _buffer.resize(n); }

	void Clear() { _buffer.clear(); }

private:
	BufferPolicy<T> _buffer;
};

template<typename T>
class SingleThreadBuffer {
	static_assert(std::is_move_constructible_v<T>, "T must be move constructible");

public:
	void push(const T& item) { _buffer.push_back(item); }
	void push(T&& item) { _buffer.push_back(std::move(item)); }

	void clear() { _buffer.clear(); }

	std::span<T> view() { return _buffer; }
	std::span<const T> view() const { return _buffer; }

	size_t size() const { return _buffer.size(); }
	void resize(size_t n) { _buffer.resize(n); }

	void swap(SingleThreadBuffer& other) noexcept
	{
		_buffer.swap(other._buffer);
	}

private:
	std::vector<T> _buffer;
};