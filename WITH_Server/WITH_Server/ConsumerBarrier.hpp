#pragma once

#include "RingBuffer.hpp"

namespace disruptor {
	class ConsumerBarrier {
	public:
		ConsumerBarrier(RingBuffer& buffer, WaitStrategy* wait, AtomicCounter* dependent)
			: _buffer(buffer), _waitStrategy(wait), _dependent(dependent) {}

		Entry& GetEntry(int64_t seq)
		{
			return _buffer.GetEntry(seq);
		}

		int64_t WaitFor(int64_t seq)
		{
			int64_t sequence = _waitStrategy->WaitFor(seq, _buffer.GetCursor(), *_dependent);
			_waitStrategy->SignalAllWhenBlocking();
			return sequence;
		}

		RingBuffer& GetRingBuffer() { return _buffer; }

	private:
		RingBuffer& _buffer;
		WaitStrategy* _waitStrategy;
		AtomicCounter* _dependent;
	};
}