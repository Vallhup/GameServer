#include "pch.h"
#include "IocpCore.h"

IocpCore::IocpCore()
{
	_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
}

IocpCore::~IocpCore()
{
	CloseHandle(_iocpHandle);
}

bool IocpCore::Register(const std::shared_ptr<IocpObject>& iocpObject)
{
	HANDLE handle = iocpObject->GetHandle();
	ULONG_PTR key = reinterpret_cast<ULONG_PTR>(iocpObject.get());

	return CreateIoCompletionPort(handle, _iocpHandle, key, 0);
}

bool IocpCore::Dispatch(unsigned int timeOutMs)
{
	std::array<OVERLAPPED_ENTRY, maxEntry> entries;
	ULONG numEntries{ 0 };

	BOOL result = GetQueuedCompletionStatusEx(_iocpHandle, entries.data(), maxEntry, &numEntries, timeOutMs, FALSE);
	if (not result) {
		DWORD error = GetLastError();
		if (WAIT_TIMEOUT == error) {
			// 정상 상황 (Debug Log)
			return true;
		}

		else {
			// 문제 상황 (Error Log)
			return false;
		}
	}

//  for (const auto& entry : entries | std::span(entries.date(), numEntries)) {
	for (const auto& entry : entries | std::views::take(numEntries)) {
		ExpOver* expOver = static_cast<ExpOver*>(entry.lpOverlapped);
		IocpObject* iocpObject = reinterpret_cast<IocpObject*>(entry.lpCompletionKey);
		DWORD ioSize = entry.dwNumberOfBytesTransferred;
		
		if (nullptr == expOver) {
			LOG_WRN("Already ShutDown");
			SetLastError(ERROR_OPERATION_ABORTED);
			continue;
		}

		if (nullptr == iocpObject) {
			LOG_WRN("Already ShutDown");
			delete expOver;
			continue;
		}

		iocpObject->Dispatch(expOver, ioSize);
	}

	return true;
}
