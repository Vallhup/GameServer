#include "pch.h"
#include "Service.h"
#include "Listener.h"

Service::Service(std::shared_ptr<IocpCore> core, int maxSessionCount)
	: _iocpCore(core), _maxSessionCount(maxSessionCount)
{
}

Service::~Service()
{
	CloseService();
}

bool Service::Start()
{
	std::cout << "Start Service\n";

	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);

	_listener = std::make_shared<Listener>();
	if (_listener == nullptr)
		return false;

	std::shared_ptr<Service> service = static_pointer_cast<Service>(shared_from_this());
	if (_listener->StartAccept(service) == false)
		return false;

	return true;
}

void Service::CloseService()
{
	// TODO : Service 종료 시 리소스 정리
}

std::shared_ptr<Session> Service::CreateSession()
{
	std::shared_ptr<GameSession> session = std::make_shared<GameSession>();
	session->SetService(shared_from_this());
	if (_iocpCore->Register(session) == false)
		return nullptr;

	return session;
}

void Service::AddSession(std::shared_ptr<Session> session)
{
	int sessionId = _sessionCount++;
	_sessions.insert(std::make_pair(sessionId, session));
	session->SetService(shared_from_this());
}

void Service::ReleaseSession(std::shared_ptr<Session> session)
{
	_sessions.unsafe_erase(session->GetSessionId());
	--_sessionCount;
}

std::shared_ptr<Service> Service::Create(std::shared_ptr<IocpCore> core, int maxSessionCount)
{
	std::shared_ptr<Service> service = std::make_shared<Service>(core, maxSessionCount);
	return service;
}