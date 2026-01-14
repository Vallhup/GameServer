#pragma once

#include <atomic>

namespace disruptor {
	class ClaimStrategy {
	public:
		virtual int64_t Claim() = 0;
	};

	class SingleProducerClaimStrategy final : public ClaimStrategy {
	public:
		virtual int64_t Claim() override
		{
			return _sequence++;
		}

	private:
		alignas(64) int64_t _sequence{ 0 };
	};

	class MultiProducerClaimStrategy final : public ClaimStrategy {
	public:
		virtual int64_t Claim() override
		{
			// Common2 RMW연산의 합의 수 = 2라고 배웠는데 과연 이렇게 하는게 thread-safe한가?
			// 요즘 fetch_add 연산은 내부적으로 Common2가 아닌 방식으로 구현된다고 한다 흠...
			return _sequence.value.fetch_add(1);
		}

	private:
		AtomicCounter _sequence;
	};
}