#pragma once

#include "IocpCore.h"
#include "Service.h"

class Listener : public IocpObject
{
public:
	Listener() = default;
	~Listener();

public:
	bool StartAccept(std::shared_ptr<ServerService> service);
	void CloseSocket();

public:
	virtual HANDLE GetHandle() override;
	virtual void Dispatch(class ExpOver* expOver, int numOfBytes = 0) override;

private:
	void doAccept(AcceptOver* acceptOver);
	void AcceptCallback(AcceptOver* acceptOver);

private:
	SOCKET _socket{ INVALID_SOCKET };
	std::vector<AcceptOver*> _acceptOvers;
	std::shared_ptr<ServerService> _service;
};

