#include "pch.h"
#include "Service.h"
#include "Listener.h"

Service::Service(std::shared_ptr<IocpCore> core, int maxSessionCount)
	: _iocpCore(core), _maxSessionCount(maxSessionCount)
{
}

Service::~Service()
{
}

void Service::CloseService()
{
}

std::shared_ptr<Session> Service::CreateSession()
{
	std::shared_ptr<Session> session = std::make_shared<Session>();
	session->SetService(shared_from_this());
	if (_iocpCore->Register(session) == false)
		return nullptr;

	return session;
}

void Service::AddSession(std::shared_ptr<Session> session)
{
	// 추후 멀티스레드 환경에서 수정될 수 있음
	++_sessionCount;
	_sessions[_sessionCount] = session;
	session->SetService(shared_from_this());
}

void Service::ReleaseSession(std::shared_ptr<Session> session)
{
	_sessions.erase(session->GetSessionId());
	--_sessionCount;
}

ServerService::ServerService(std::shared_ptr<IocpCore> core, int maxSessionCount)
	: Service(core, maxSessionCount)
{
}

std::shared_ptr<ServerService> ServerService::Create(std::shared_ptr<IocpCore> core, int maxSessionCount)
{
	std::shared_ptr<ServerService> service = std::make_shared<ServerService>(core, maxSessionCount);
	return service;
}

bool ServerService::Start()
{
	std::cout << "Start Service\n";

	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);

	_listener = std::make_shared<Listener>();
	if (_listener == nullptr)
		return false;
	
	std::shared_ptr<ServerService> service = static_pointer_cast<ServerService>(shared_from_this());
	if (_listener->StartAccept(service) == false)
		return false;

	return true;
}

void ServerService::CloseService()
{
	Service::CloseService();
}
