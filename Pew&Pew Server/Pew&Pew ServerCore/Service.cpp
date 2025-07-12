#include "pch.h"
#include "Service.h"

bool Service::Init()
{
	WSADATA WSAData;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
		LOG_ERR("WSAStartup Error");
		return false;
	}

	if (nullptr == _listener) {
		_listener = std::make_shared<Listener>(shared_from_this());
	}

	if (_listener->Init() == false) {
		LOG_ERR("Listener StartAccept filed");
		return false;
	}

	return true;
}

void Service::Run()
{
	_running = true;

	while (_running) {
		fd_set readSet;
		FD_ZERO(&readSet);

		FD_SET(_listener->GetSocket(), &readSet);
		SOCKET maxSocket = _listener->GetSocket();

		for (auto& [id, session] : _sessions) {
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
			AcceptSession();
		}

		std::vector<int> closed;
		for (auto& [id, session] : _sessions) {
			if (FD_ISSET(session->GetSocket(), &readSet)) {
				if (not session->Recv()) {
					closed.push_back(id);
				}
			}
		}

		for (int id : closed) {
			CloseSession(id);
		}
	}
}

void Service::Stop()
{
	_running = false;

	for (auto& [id, session] : _sessions) {
		session->DisConnect();
	}

	_sessions.clear();
}

void Service::BroadCast(const std::vector<char>& packet, int exceptId)
{
	for (auto& [id, session] : _sessions) {
		if (id == exceptId) continue;
		session->Send(packet);
	}
}

void Service::AcceptSession()
{
	SOCKET clientSocket = _listener->Accept();
	if (INVALID_SOCKET == clientSocket) {
		return;
	}

	int sessionId = GenerateSessionId();
	if (sessionId == -1) {
		LOG_ERR("Error : Session[-1]");
		closesocket(clientSocket);
		return;
	}

	u_long mode{ 1 };
	ioctlsocket(clientSocket, FIONBIO, &mode);

	std::string name = "Player" + std::to_string(sessionId);

	auto session = std::make_shared<Session>(sessionId, clientSocket);
	auto character = std::make_shared<Character>(sessionId, name);
	session->SetCharacter(character);

	_sessions.insert(std::make_pair(sessionId, session));
	_characters.insert(std::make_pair(sessionId, character));

	session->OnConnect(this);

	// 3. 나에게 남
	for (auto& [id, sess] : _sessions) {
		if (session->GetId() == id) continue;

		LOG_DBG("Session[%d] Id : %d", sess->GetId());
		session->Send(PacketFactory::SCAddPacket(*sess->GetCharacter()));
	}
}

void Service::CloseSession(int id)
{
	if (_sessions.contains(id)) {
		_sessions[id]->DisConnect();
		_sessions.erase(id);
		_reusableSessionIds.push_back(id);
	}
}

Service::Service()
{
	_reusableSessionIds.resize(64);
	std::iota(_reusableSessionIds.begin(), _reusableSessionIds.end(), 0);
}

Service::~Service()
{
	Stop();
}

int Service::GenerateSessionId()
{
	if (_reusableSessionIds.empty()) {
		return -1;
	}

	int id = _reusableSessionIds.back();
	_reusableSessionIds.pop_back();
	return id;
}
