#include "pch.h"
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
		_visualizer->Start();

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

		_visualizer->Stop();

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
	using namespace std::chrono;

	int x{ 0 };
	MSG msg;
	auto prev = high_resolution_clock::now();
	while (_running.load()) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) _running = false;
			else {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		_visualizer->Render();

		const auto now = high_resolution_clock::now();
		const float deltaTime = duration<float>(now - prev).count();
		prev = now;

		if (x++ < 150) {
			_clientMng->AdjustClients();
		}
		_clientMng->OnTick(deltaTime);

		std::this_thread::sleep_for(100ms);
	}
}
