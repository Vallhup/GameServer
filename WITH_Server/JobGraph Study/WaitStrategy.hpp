#pragma once

#include <chrono>
#include <mutex>
#include <condition_variable>

#include "AtomicCounter.hpp"

namespace disruptor {
	class WaitStrategy {
	public:
		virtual int64_t WaitFor(int64_t seq, const AtomicCounter& cursor, const AtomicCounter& dependent) = 0;
		virtual void SignalAllWhenBlocking() { }
	};

	class BusyWaitStrategy final : public WaitStrategy {
	public:
		virtual int64_t WaitFor(int64_t seq, const AtomicCounter& cursor, const AtomicCounter& dependent) override
		{
			int64_t available;
			while ((available = std::min<int64_t>(cursor.value, dependent.value)) < seq);
			return available;
		}
	};

	class YieldWaitStrategy final : public WaitStrategy {
	public:
		virtual int64_t WaitFor(int64_t seq, const AtomicCounter& cursor, const AtomicCounter& dependent) override
		{
			volatile int loopCount{ 20 };
			int64_t available;
			while ((available = std::min<int64_t>(cursor.value, dependent.value)) < seq)
			{
				if (--loopCount <= 0)
				{
					std::this_thread::yield();
					loopCount = 20;
				}
			}

			return available;
		}
	};

	class SleepWaitStrategy final : public WaitStrategy {
	public:
		virtual int64_t WaitFor(int64_t seq, const AtomicCounter& cursor, const AtomicCounter& dependent) override
		{
			using namespace std::chrono;

			volatile int loopCount{ 20 };
			int64_t available;
			while ((available = std::min<int64_t>(cursor.value, dependent.value)) < seq)
			{
				if (--loopCount <= 0)
					std::this_thread::sleep_for(1ms);
			}

			return available;
		}
	};

	class BlockingWaitStrategy final : public WaitStrategy {
	public:
		virtual int64_t WaitFor(int64_t seq, const AtomicCounter& cursor, const AtomicCounter& dependent) override
		{
			std::unique_lock lock{ _mtx };

			int64_t available;
			while ((available = std::min<int64_t>(cursor.value, dependent.value)) < seq)
			{
				_cv.wait(lock);
			}

			return available;
		}

		virtual void SignalAllWhenBlocking() override
		{
			std::lock_guard lock{ _mtx };
			_cv.notify_all();
		}

	private:
		std::mutex _mtx;
		std::condition_variable _cv;
	};
}
