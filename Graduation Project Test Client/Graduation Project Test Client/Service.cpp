#include "Service.h"
#include "ClientManager.h"

Service::Service()
{
	_iocpCore = std::make_unique<IocpCore>();
	_clientMng = std::make_unique<ClientManager>(3000, *this);
	_visualizer = std::make_unique<Visualizer>(800, 600, false);
}

void Service::Start()
{
	setlocale(LC_ALL, "korean");

	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
		return;
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
		
		MainLoop();
	}
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

void Service::RegisterClient(const std::shared_ptr<Client>& client)
{
	_iocpCore->Register(client);
}

void Service::MainLoop()
{
	for (int i = 0; i < 20; ++i) {
		_clientMng->AdjustClients();
	}

	MSG msg;
	while (_running.load()) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) _running.store(false);
			else {
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
		}

		else {
			_clientMng->OnTick();
			_visualizer->Render();

			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
}
