#include "pch.h"
#include "Service.h"
#include "SessionManager.h"

Service::Service()
{
	_listener = std::make_unique<Listener>();

	_charMng = std::make_unique<CharacterManager>();
	_projMng = std::make_unique<ProjectileManager>();
	_timerMng = std::make_unique<TimerManager>();

	_collMng = std::make_unique<CollisionManager>(*this);
	_sessMng = std::make_unique<SessionManager>(*this);
	_roomMng = std::make_unique<RoomManager>(*this);
	_gameLogic = std::make_unique<GameLogic>(*this);
}

Service::~Service()
{
	Stop();
}

bool Service::Init()
{
	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
		LOG_ERR("WSAStartup Error");
		return false;
	}

	if (not _listener->Init()) {
		LOG_ERR("Listener StartAccept failed");
		return false;
	}

	_timerMng->AddRepeatedTask([this](float deltaTime) { _gameLogic->LogicUpdate(deltaTime); }, 4.16f);
	_timerMng->AddRepeatedTask([this](float) { _gameLogic->NetworkUpdate(); }, 16.66f);

	return true;
}


void Service::Start()
{
	_running = true;

	_timerMng->Start();
	while (_running) {
		fd_set readSet;
		FD_ZERO(&readSet);

		FD_SET(_listener->GetSocket(), &readSet);
		SOCKET maxSocket = _listener->GetSocket();

		for (auto& session : _sessMng->GetSessionList()) {
			SOCKET socket = session->GetSocket();
			FD_SET(socket, &readSet);

			if (socket > maxSocket) {
				maxSocket = socket;
			}
		}

		timeval timeout = { 0, 10000 };
		if (SOCKET_ERROR == select(static_cast<int>(maxSocket + 1), &readSet, nullptr, nullptr, &timeout)) {
			continue;
		}

		if (FD_ISSET(_listener->GetSocket(), &readSet)) {
			SOCKET clientSocket = _listener->Accept();
			_sessMng->AcceptSession(clientSocket);
		}

		std::vector<int> closed;
		for (auto& session : _sessMng->GetSessionList()) {
			if (FD_ISSET(session->GetSocket(), &readSet)) {
				if (not session->Recv()) {
					closed.push_back(session->GetId());
				}
			}
		}

		for (int id : closed) {
			_sessMng->CloseSession(id);
		}
	}
}

void Service::Stop()
{
	_running = false;

	for (auto& session : _sessMng->GetSessionList()) {
		session->DisConnect();
	}

	_timerMng->Stop();
}

void Service::BroadCast(const std::vector<char>& packet, int exceptId)
{
	for (auto& session : _sessMng->GetSessionList()) {
		if (session->GetId() == exceptId) continue;
		session->Send(packet);
	}
}

float Service::GetNowTime()
{
	using namespace std::chrono;
	static const auto start = high_resolution_clock::now();
	auto now = high_resolution_clock::now();
	return duration<float>(now - start).count();
}
