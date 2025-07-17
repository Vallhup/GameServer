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

	if (nullptr == _timerManager) {
		_timerManager = std::make_shared<TimerManager>();
	}

	if (nullptr == _collisionManager) {
		_collisionManager = std::make_shared<CollisionManager>();
	}

	_timerManager->Register([this](float delta) { this->Tick(delta); });

	return true;
}

void Service::Run()
{
	_running = true;

	_timerManager->Start();
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
	_characters.clear();
	_projectiles.clear();

	_timerManager->Stop();
}

void Service::Tick(float deltaTime)
{
	std::vector<std::shared_ptr<Character>> characters;
	characters.reserve(64 - _reusableSessionIds.size());
	{
		std::shared_lock lock{ _characterMutex };
		for (auto& [id, character] : _characters) {
			characters.push_back(character);
		}
	}

	for (auto& character : characters) {
		bool moved = character->Move(deltaTime);
		if (moved or character->GetAngleChange()) {
			BroadCast(PacketFactory::SCMovePacket(*character));
			character->ResetAngleChange();
		}

		character->Attack(GetNowTime(), this);
	}

	std::vector<std::shared_ptr<Projectile>> projectiles;
	projectiles.reserve(characters.size() * 3);
	{
		std::shared_lock lock{ _projectileMutex };
		for (auto& [id, projectile] : _projectiles) {
			projectiles.push_back(projectile);
		}
	}

	for (auto& projectile : projectiles) {
		projectile->Update(deltaTime, this);
		BroadCast(PacketFactory::SCMovePacket(*projectile));
	}

	_collisionManager->Update(projectiles, characters, this);
}

void Service::AddProjectile(int sessionId, vec3 direction)
{
	std::shared_ptr<Character> character;
	{
		std::shared_lock lock{ _characterMutex };
		character = _characters.find(sessionId)->second;
	}

	int projId = _nextProjectileId++;
	std::shared_ptr<Projectile> projectile = std::make_shared<Projectile>(projId, sessionId, character->GetPosition(), direction, 10);
	{
		std::unique_lock lock{ _projectileMutex };
		_projectiles.insert(std::make_pair(projId, projectile));
	}

	BroadCast(PacketFactory::SCAddPacket(*projectile));
}

void Service::RemoveProjectile(int projId)
{
	std::unique_lock lock{ _projectileMutex };
	_projectiles.erase(projId);
}

void Service::BroadCast(const std::vector<char>& packet, int exceptId)
{
	std::vector<std::shared_ptr<Session>> sessions;
	{
		std::shared_lock lock{ _sessionMutex };
		for (auto& [id, session] : _sessions) {
			if (id != exceptId) {
				sessions.push_back(session);
			}
		}
	}

	for (auto& session : sessions) {
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
	session->SetService(this);

	{
		std::unique_lock lock{ _sessionMutex };
		_sessions.insert(std::make_pair(sessionId, session));
	}

	{
		std::unique_lock lock{ _characterMutex };
		_characters.insert(std::make_pair(sessionId, character));
	}

	session->OnConnect();

	BroadCast(PacketFactory::SCAddPacket(*character));

	{
		std::shared_lock lock{ _sessionMutex };

		// 새로운 플레이어에게 기존 플레이어들의 정보 보내기
		for (auto& [id, sess] : _sessions) {
			if (session->GetId() == id) continue;
			session->Send(PacketFactory::SCAddPacket(*sess->GetCharacter()));
		}
	}
}

void Service::CloseSession(int id)
{
	std::shared_ptr<Session> session;
	std::shared_ptr<Character> character;

	{
		std::unique_lock lock{ _sessionMutex };
		auto it = _sessions.find(id);
		if (it != _sessions.end()) {
			session = it->second;
			_sessions.erase(it);
		}
	}

	{
		std::unique_lock lock{ _characterMutex };
		auto it = _characters.find(id);
		if (it != _characters.end()) {
			character = it->second;
			_characters.erase(it);
		}
	}

	if (session and character) {
		BroadCast(PacketFactory::SCRemovePacket(*character));

		session->DisConnect();
		_reusableSessionIds.push_back(id);
	}
}

Service::Service()
{
	_nextProjectileId.store(64);
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
