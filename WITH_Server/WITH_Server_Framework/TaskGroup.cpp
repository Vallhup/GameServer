#include "pch.h"
#include "TaskGroup.h"

bool TaskGroup::TryAcquireTaskSlot()
{
	if (!_accepting.load())
		return false;

	if (_cancelRequested.load() &&
		_mode == TaskGroupMode::StopOnFirstFailure)
	{
		return false;
	}

	_pending.fetch_add(1);

	if (!_accepting.load())
	{
		_pending.fetch_sub(1);
		return false;
	}

	if (_cancelRequested.load() &&
		_mode == TaskGroupMode::StopOnFirstFailure)
	{
		_pending.fetch_sub(1);
		return false;
	}

	return true;
}

void TaskGroup::RollbackTaskSlot() noexcept
{
	const int prev = _pending.fetch_sub(1);
	assert(prev > 0);

	if (prev == 1)
	{
		std::lock_guard lock{ _mtx };
		_cv.notify_all();
	}
}

void TaskGroup::RecordException(std::exception_ptr eptr)
{
	{
		std::lock_guard lock{ _mtx };
		if (!_firstException)
			_firstException = eptr;
	}

	_failed.store(true);
	_failedCount.fetch_add(1);

	if (_mode == TaskGroupMode::StopOnFirstFailure)
		_cancelRequested.store(true);
}

void TaskGroup::Release(bool executed, bool skipped, bool failed)
{
	if (executed)
		_executed.fetch_add(1);

	if (skipped)
		_skipped.fetch_add(1);

	if (failed)
	{
		// failedCount는 RecordException에서 올리므로 중복 증가시키지 않는다.
	}

	if (_pending.fetch_sub(1) == 1)
	{
		std::lock_guard lock{ _mtx };
		_cv.notify_all();
	}
}

void TaskGroup::RecordException(std::exception_ptr eptr)
{
	{
		std::lock_guard lock{ _mtx };
		if (!_firstException)
			_firstException = eptr;
	}

	_failed.store(true);

	if (_mode == TaskGroupMode::StopOnFirstFailure)
		_cancelRequested.store(true);
}

void TaskGroup::CloseSubmit()
{
	_accepting.store(false);

	if (_pending.load() == 0)
	{
		std::lock_guard lock{ _mtx };
		_cv.notify_all();
	}
}

void TaskGroup::Wait()
{
	std::unique_lock lock{ _mtx };
	_cv.wait(lock,
		[&]()
		{
			return
				!_accepting.load() &&
				_pending.load() == 0;
		});

	if (_firstException)
		std::rethrow_exception(_firstException);
}