#include "pch.h"
#include "Service.h"

Service::Service()
{
	_listener = std::make_shared<Listener>(*this);

	_iocpCore = std::make_unique<IocpCore>();

	_sessMng = std::make_unique<SessionManager>(*this);
	_gameWorld = std::make_unique<GameWorld>(*this);
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

		const unsigned int threadCount = std::thread::hardware_concurrency();

		_tickThread = std::thread([this]() { TickFunc(); });
		_iocpWorker.Start(2, [this]() { IocpFunc();});
		_logicWorker.Start(threadCount - 3, [this]() { LogicFunc();});

		return true;
	}

	return false;
}

void Service::Stop()
{
	bool expected{ true };
	if (_running.compare_exchange_strong(expected, false)) {
		_listener->Stop();

		for (size_t i = 0; i < 2; ++i) {
			PostQueuedCompletionStatus(_iocpCore->GetHandle(), 0, 0, nullptr);
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

void Service::TickFunc()
{
	using namespace std::chrono;

	auto prev = high_resolution_clock::now();
	while (_running.load()) {
		auto now = high_resolution_clock::now();
		float deltaTime = duration<float>(now - prev).count();
		prev = now;

		_gameWorld->Update(deltaTime);

		// TEMP : 60Hz
		std::this_thread::sleep_for(16ms);
	}
}

void Service::IocpFunc()
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
}

void Service::LogicFunc()
{
	while (_running.load()) {
		std::shared_ptr<Job> job;
		if (_jobQueue.TryPop(job)) {
			job->Execute();
		}
	}
}
