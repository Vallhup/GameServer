#include "pch.h"
#include "Service.h"
#include "Listener.h"

Service::Service(std::shared_ptr<IocpCore> core, int maxSessionCount)
	: _iocpCore(core), _maxSessionCount(maxSessionCount), _chatManager(*this)
{
}

bool Service::Start()
{
	LOG_INF("Start Service");

	_running.store(true);

	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);

	_listener = std::make_shared<Listener>();
	if (_listener == nullptr) {
		LOG_ERR("Listener allocation failed");
		return false;
	}

	std::shared_ptr<Service> service = shared_from_this();
	if (_listener->StartAccept(service) == false) {
		LOG_ERR("Listener StartAccept filed");
		return false;
	}

	unsigned int threadCount = std::thread::hardware_concurrency();
	_workers.reserve(threadCount);
	for (unsigned int i = 0; i < threadCount; ++i) {
		_workers.emplace_back([this]()
			{
				while (_running.load()) {
					if (not _iocpCore->Dispatch()) {
						int error = WSAGetLastError();

						if (_running.load() and (error != ERROR_OPERATION_ABORTED)) {
							LOG_ERR("Dispatch error: &d", error);
						}

						break;
					}
				}
			});
	}

	return true;
}

void Service::CloseService()
{
	// 0) _running Flag 설정
	_running.store(false);

	// 1) Accept 종료
	_listener->StopAccept();

	// 2) Worker thread join
	for (size_t i = 0; i < _workers.size(); ++i) {
		PostQueuedCompletionStatus(_iocpCore->GetHandle(), 0, 0, nullptr);
	}

	for (std::thread& worker : _workers) {
		if (worker.joinable()) {
			worker.join();
		}
	}
	_workers.clear();

	// 3) Session 정리
	for (auto& [id, session] : _sessions) {
		SessionPtr p = session.load();
		if (nullptr != p) {
			CancelIoEx(p->GetHandle(), nullptr);
		}
	}
	_sessions.clear();

	// 4) Listener 해제
	_listener.reset();

	// 5) IOCP Handle 해제
	_iocpCore.reset();

	// 6) WinSock 정리
	WSACleanup();
}

SessionPtr Service::CreateSession()
{
	std::shared_ptr<GameSession> session = std::make_shared<GameSession>();
	session->SetService(shared_from_this());
	if (_iocpCore->Register(session) == false)
		return nullptr;

	return session;
}

void Service::AddSession(const std::shared_ptr<GameSession> session)
{
	int sessionId = _sessionCount++;
	session->SetId(sessionId);
	_sessions.insert(std::make_pair(sessionId, session));
	session->SetService(shared_from_this());
}

void Service::ReleaseSession(const std::shared_ptr<GameSession> session)
{
	short sessionId = session->_id;

	SC_REMOVE_PLAYER_PACKET remove;
	remove.id = sessionId;
	remove.size = sizeof(remove);
	remove.type = SC_REMOVE_PLAYER;

	for (auto& [id, atomicSession] : _sessions) {
		if (auto p = atomicSession.load()) {
			if (p->GetSessionId() != sessionId) {
				p->Send(Serialize(remove));
			}
		}
	}

	_sessions.at(sessionId) = nullptr;
	--_sessionCount;

	LOG_INF("Session %d released", sessionId);
}

void Service::enterSector(const std::shared_ptr<GameSession>& session)
{
	auto [sx, sy] = Sector::getSector(session->_pos._xPos, session->_pos._yPos);
	_sectors[sx][sy].addClient(session->_id);
}

void Service::leaveSector(const std::shared_ptr<GameSession>& session)
{
	auto [sx, sy] = Sector::getSector(session->_pos._xPos, session->_pos._yPos);
	_sectors[sx][sy].removeClient(session->_id);
}

std::unordered_set<int> Service::collectVisibleClients(const std::shared_ptr<GameSession>& session) const
{
	auto [xRange, yRange] = Sector::getSectorRange(session->_pos._xPos, session->_pos._yPos);
	std::unordered_set<int> result;

	for (int sx = xRange.first; sx <= xRange.second; ++sx) {
		for (int sy = yRange.first; sy <= yRange.second; ++sy) {
			_sectors[sx][sy].collectClient(result);
		}
	}

	return result;
}

void Service::OnChatRequest(short senderId, const char* msg)
{
	_chatManager.HandleMessage(senderId, msg);
}

void Service::Broadcast(const std::vector<char>& buf)
{
	for (auto& [id, session] : _sessions) {
		if (GameSessionPtr p = session.load()) {
			p->Send(buf);
		}
	}
}

std::shared_ptr<Service> Service::Create(std::shared_ptr<IocpCore> core, int maxSessionCount)
{
	std::shared_ptr<Service> service = std::make_shared<Service>(core, maxSessionCount);
	return service;
}