#include "pch.h"
#include "IocpCore.h"

int IocpCore::objectId{ 0 };

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
	LOG_DBG("Enter Register IocpCore");

	iocpObject->SetSessionId(objectId);
	HANDLE result = CreateIoCompletionPort(iocpObject->GetHandle(), _iocpHandle, objectId++, 0);
	if (result == NULL) {
		LOG_INF("Register failed");
		return false;
	}

	return true;
}

bool IocpCore::Dispatch(unsigned int timeoutMs)
{
	LOG_DBG("Enter Dispatch IocpCore");

	DWORD ioSize{ 0 };
	ULONG_PTR key{ 0 };
	ExpOver* expOver{ nullptr };

	BOOL result = GetQueuedCompletionStatus(_iocpHandle, &ioSize, &key, reinterpret_cast<LPOVERLAPPED*>(&expOver), timeoutMs);
	if ((not result) and (WAIT_TIMEOUT == WSAGetLastError())) {
		LOG_DBG("Dispatch timeout");
		return true;
	}

	if (nullptr == expOver) {
		LOG_INF("Dispatch received shutdown signal");
		SetLastError(ERROR_OPERATION_ABORTED);
		return false;
	}

	std::shared_ptr<IocpObject> iocpObject = expOver->_owner;
	if (nullptr == iocpObject) {
		/*if (expOver->_operationType == OperationType::NpcMove) {
			NpcOver* npcOver = static_cast<NpcOver*>(expOver);
			if (auto service = _service.lock()) {
				auto npc = static_pointer_cast<NPC>(service->FindObject(npcOver->_npcId));
				if (nullptr != npc) {
					npc->_timerPending.store(false);

					if (not npc->_isActive.load()) {
						delete expOver;
						return true;
					}

					npc->OnTimer();

					delete expOver;
				}
			}

			return true;
		}*/

		LOG_ERR("ExpOver has no owner!");
		return false;
	}

	LOG_DBG("Dispatch success: key=%11u", (unsigned long long)key);
	iocpObject->Dispatch(expOver, ioSize);

	return true;
}