#pragma once

enum class TaskGroupMode : uint8_t
{
	ContinueOnFailure,
	StopOnFirstFailure
};

class TaskGroup final {
public:
	explicit TaskGroup(TaskGroupMode mode = TaskGroupMode::StopOnFirstFailure)
		: _mode(mode) {}

	TaskGroup(const TaskGroup&) = delete;
	TaskGroup& operator=(const TaskGroup&) = delete;

	// TEMP : 아직 완전한 lock-step close barrier 아님
	bool TryAcquireTaskSlot();
	void RollbackTaskSlot() noexcept;

	void Release(bool executed, bool skipped, bool failed);

	void RecordException(std::exception_ptr eptr);
	void CloseSubmit();
	void Wait();

	TaskGroupMode GetMode() const noexcept { return _mode; }

	bool IsAccepting() const noexcept { return _accepting.load(); }
	bool IsCancelRequested() const noexcept { return _cancelRequested.load(); }
	bool HasFailed() const noexcept { return _failed.load(); }

	int PendingCount() const noexcept { return _pending.load(); }
	uint32_t ExecutedCount() const noexcept { return _executed.load(); }
	uint32_t SkippedCount() const noexcept { return _skipped.load(); }
	uint32_t FailedCount() const noexcept { return _failedCount.load(); }

	std::exception_ptr FirstException() const
	{
		std::lock_guard lock{ _mtx };
		return _firstException;
	}
	
private:
	std::atomic<int> _pending{ 0 };
	std::atomic<bool> _accepting{ true };
	std::atomic<bool> _cancelRequested{ false };
	std::atomic<bool> _failed{ false };

	std::atomic<uint32_t> _executed{ 0 };
	std::atomic<uint32_t> _skipped{ 0 };
	std::atomic<uint32_t> _failedCount{ 0 };

	TaskGroupMode _mode{ TaskGroupMode::StopOnFirstFailure };

	std::mutex _mtx;
	std::condition_variable _cv;
	std::exception_ptr _firstException{ nullptr };
};
