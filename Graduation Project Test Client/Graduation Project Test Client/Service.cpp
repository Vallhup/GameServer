#include "Service.h"
#include "ClientManager.h"

Service::Service()
{
	_iocpCore = std::make_unique<IocpCore>();
	_clientMng = std::make_unique<ClientManager>(3000, *this);
}

bool Service::Start()
{
	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
		return false;
	}

	bool expected{ false };
	if (_running.compare_exchange_strong(expected, true)) {
		const unsigned int threadCount = std::thread::hardware_concurrency();
		_workers.resize(threadCount);

		for (auto& worker : _workers) {
			worker = std::thread([&]()
				{
					while (_running.load()) {
						if (not _iocpCore->Dispatch()) {
							int error = WSAGetLastError();

							if (_running.load()) {
								std::cout << "IOCP Dispatch error : " << error << std::endl;
							}

							break;
						}

					}
				});
		}

		_connectThread = std::thread([&]()
			{

				while(true) {
					_clientMng->AdjustClients();
					_clientMng->OnTick();
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				}
			});

		return true;
	}

	return false;
}

void Service::Stop()
{
	bool expected{ true };
	if (_running.compare_exchange_strong(expected, false)) {
		for (size_t i = 0; i < _workers.size(); ++i) {
			PostQueuedCompletionStatus(_iocpCore->GetHandle(), 0, 0, nullptr);
		}

		for (auto& worker : _workers) {
			if (worker.joinable()) {
				worker.join();
			}
		}

		WSACleanup();
	}
}

void Service::RegisterClient(Client* client)
{
	_iocpCore->Register(client);
}
