#include "pch.h"
#include "IocpHandle.h"

IocpHandle::IocpHandle() noexcept
{
	_handle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
}

IocpHandle::~IocpHandle() noexcept
{
	if (_handle != INVALID_HANDLE_VALUE)
		::CloseHandle(_handle);
}

bool IocpHandle::Register(HANDLE fileHandle, ULONG_PTR key) noexcept
{
	return ::CreateIoCompletionPort(fileHandle, _handle, key, 0) == _handle;
}

void IocpHandle::WakeWorker() noexcept
{
	::PostQueuedCompletionStatus(_handle, 0, 0, nullptr);
}
