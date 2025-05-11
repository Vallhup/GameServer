#include "pch.h"
#include "IocpCore.h"

int IocpCore::clientId{ 0 };

IocpCore::IocpCore()
{
	_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
}

IocpCore::~IocpCore()
{
	CloseHandle(_iocpHandle);
}

bool IocpCore::Register(std::shared_ptr<IocpObject> iocpObject)
{
	std::cout << "Start Register IocpCore\n";

	iocpObject->SetId(clientId);
	HANDLE result = CreateIoCompletionPort(iocpObject->GetHandle(), _iocpHandle, clientId++, 0);
	if (result == NULL) {
		std::cout << "IOCP failed\n";
		return false;
	}

	return true;
}

bool IocpCore::Dispatch(unsigned int timeoutMs)
{
	std::cout << "Dispatch IocpCore\n";

	DWORD ioSize{ 0 };
	ULONG_PTR key{ 0 };
	ExpOver* expOver{ nullptr };

	BOOL result = GetQueuedCompletionStatus(_iocpHandle, &ioSize, &key, reinterpret_cast<LPOVERLAPPED*>(&expOver), timeoutMs);
	if ((not result) and (WAIT_TIMEOUT == WSAGetLastError())) {
		return true;
	}

	if (nullptr == expOver) {
		SetLastError(ERROR_OPERATION_ABORTED);
		return false;
	}


	std::shared_ptr<IocpObject> iocpObject = expOver->_owner;
	iocpObject->Dispatch(expOver, ioSize);

	return true;
}
