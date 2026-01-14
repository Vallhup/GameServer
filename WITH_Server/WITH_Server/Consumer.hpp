#pragma once

#include <memory>

#include "ConsumerBarrier.hpp"
#include "BatchHandler.hpp"
#include "RingBuffer.hpp"

namespace disruptor {
	class Consumer {
	public:
		Consumer(ConsumerBarrier& barrier) : _sequence(-1), _barrier(barrier)
		{
			_batchHandler = std::make_unique<TestHandler>();
			_barrier.GetRingBuffer().AddGatingSequence(&_sequence);
		}

		Consumer(ConsumerBarrier& barrier, std::unique_ptr<BatchHandler> handler) 
			: _sequence(-1), _barrier(barrier), _batchHandler(std::move(handler)) 
		{
			_barrier.GetRingBuffer().AddGatingSequence(&_sequence);
		}
	
		void ConsumeEntries()
		{
			// TEMP : 성능 테스트용 무한 루프
			while (true)
			{
				int64_t available = _barrier.WaitFor(_sequence.value + 1);
				while (_sequence.value < available)
				{
					++_sequence.value;
					_batchHandler->OnAvailable(_barrier.GetEntry(_sequence.value));
				}
				_batchHandler->OnEndOfBatch();
			}
		}

	private:
		AtomicCounter _sequence;
		ConsumerBarrier& _barrier;
		std::unique_ptr<BatchHandler> _batchHandler;
	};
}

