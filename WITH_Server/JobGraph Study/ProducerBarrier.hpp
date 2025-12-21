#pragma once

#include <memory>
#include "RingBuffer.hpp"
#include "ClaimStrategy.hpp"
#include "WaitStrategy.hpp"

struct Entry;

namespace disruptor {
	class ProducerBarrier {
	public:
		ProducerBarrier(RingBuffer& buffer, WaitStrategy* wait) : _buffer(buffer), _waitStrategy(wait)
		{
			_claimStrategy = std::make_unique<SingleProducerClaimStrategy>();
		}

		ProducerBarrier(RingBuffer& buffer, WaitStrategy* wait, std::unique_ptr<ClaimStrategy> claim) 
			: _buffer(buffer), _waitStrategy(wait), _claimStrategy(std::move(claim)) { }

		ProducerBarrier(const ProducerBarrier&) = delete;
		ProducerBarrier& operator=(const ProducerBarrier&) = delete;

		Entry& GetEntry(int64_t seq)
		{
			return _buffer.GetEntry(seq);
		}

		int64_t Claim()
		{
			// TEMP : O(n) [n = Consumer 수]인 GetMinGatingSequence의 호출을 최소화 하기 위해
			//        이전에 계산한 MinGatingSequence를 Caching
			int64_t seq = _claimStrategy->Claim();
			int64_t wrapPoint = seq - _buffer.GetSize();
			while (wrapPoint > _buffer.GetMinGatingSequence())
				std::this_thread::yield();

			return seq;
		}

		void Publish(int64_t seq)
		{
			const int64_t beforeSeq = seq - 1;
			while (_buffer.GetCursor().value < beforeSeq)
				std::this_thread::yield();
				//_waitStrategy->WaitFor(beforeSeq, _buffer.GetCursor(), _buffer.GetCursor());

			_buffer.SetCursor(seq);
			_waitStrategy->SignalAllWhenBlocking();
		}

	private:
		RingBuffer& _buffer;
		WaitStrategy* _waitStrategy;
		std::unique_ptr<ClaimStrategy> _claimStrategy;
	};
}

