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
	return CreateIoCompletionPort(iocpObject->GetHandle(), _iocpHandle, clientId++, 0);
}

bool IocpCore::Dispatch(unsigned int timeoutMs)
{
	DWORD ioSize{ 0 };
	ULONG_PTR key{ 0 };
	ExpOver* expOver{ nullptr };

	if (GetQueuedCompletionStatus(_iocpHandle, &ioSize, &key,
		reinterpret_cast<LPOVERLAPPED*>(&expOver), timeoutMs))
	{
		std::shared_ptr<IocpObject> iocpObject = expOver->_owner;
		iocpObject->Dispatch(expOver, ioSize);
	}

	else {
		int error = WSAGetLastError();
		switch (error) {
		case WAIT_TIMEOUT:
			return false;

		default:
			std::shared_ptr<IocpObject> iocpObject = expOver->_owner;
			iocpObject->Dispatch(expOver, ioSize);
			break;
		}
	}

	return true;
}
