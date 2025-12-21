#pragma once

#include "Entry.hpp"

namespace disruptor {
	class BatchHandler {
	public:
		virtual void OnAvailable(Entry& entry) = 0;
		virtual void OnEndOfBatch() = 0;

	protected:
		virtual void OnCompletion() = 0;
	};

	class TestHandler final : public BatchHandler {
	public:
		virtual void OnAvailable(Entry& entry) override
		{
			processed.fetch_add(1);
		}

		virtual void OnEndOfBatch() override
		{

		}

		std::atomic<int64_t> processed{ 0 };

	private:
		virtual void OnCompletion() override
		{

		}
	};
}