#include "pch.h"
#include "Service.h"

Service::Service()
{
	_listener = std::make_shared<Listener>(*this);

	_iocpCore = std::make_unique<IocpCore>();

	_eventMng = std::make_unique<EventManager>();

	_sessMng = std::make_unique<SessionManager>(*this);
	_gameLogic = std::make_unique<GameLogic>(*this);
	_gameWorld = std::make_unique<GameWorld>();
}

bool Service::Init()
{
	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
		LOG_ERR("WSAStartup Error");
		return false;
	}

	if (not _listener->Init()) {
		LOG_ERR("Listener Init failed");
		return false;
	}

	if (not _iocpCore->Register(_listener)) {
		LOG_ERR("IocpCore Listener Register Failed");
		return false;
	}

	return true;
}


bool Service::Start()
{
	bool expected{ false };
	if (_running.compare_exchange_strong(expected, true)) {
		if (not _listener->Start()) {
			LOG_ERR("Listener StartAccept failed");
			return false;
		}

		if (not _eventMng->Start()) {
			LOG_ERR("EventManager already Start");
			return false;
		}

		const unsigned int threadCount = std::thread::hardware_concurrency();
		_workers.reserve(threadCount);

		for (unsigned int i = 0; i < threadCount; ++i) {
			_workers.emplace_back([this]()
				{
					while (_running.load()) {
						if (not _iocpCore->Dispatch()) {
							int error = WSAGetLastError();

							if (_running.load()) {
								if (error == WSAECONNRESET || error == WSAENOTCONN || error == ERROR_NETNAME_DELETED || error == WSA_OPERATION_ABORTED) {
									LOG_WRN("IOCP Dispatch expected error: %d", error);
								}
								else {
									LOG_ERR("IOCP Dispatch critical error: %d", error);
								}
							}

							break;
						}
					}
				});
		}

		return true;
	}

	return false;
}

void Service::Stop()
{
	bool expected{ true };
	if (_running.compare_exchange_strong(expected, false)) {
		_listener->Stop();
		_eventMng->Stop();

		for (size_t i = 0; i < _workers.size(); ++i) {
			PostQueuedCompletionStatus(_iocpCore->GetHandle(), 0, 0, nullptr);
		}

		for (std::thread& worker : _workers) {
			if (worker.joinable()) {
				worker.join();
			}
		}

		WSACleanup();
	}
}

void Service::BroadCast(const std::vector<char>& packet, int exceptId)
{
	// TODO : Session BroadCast
}

float Service::GetNowTime()
{
	using namespace std::chrono;
	static const auto start = high_resolution_clock::now();
	auto now = high_resolution_clock::now();
	return duration<float>(now - start).count();
}
