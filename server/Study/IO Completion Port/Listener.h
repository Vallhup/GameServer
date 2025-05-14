#pragma once

class Service;
class ExpOver;

class Listener : public IocpObject
{
public:
	Listener() = default;
	~Listener();

public:
	bool StartAccept(std::shared_ptr<Service> service);
	void StopAccept();
	void CloseSocket();

public:
	virtual HANDLE GetHandle() override;
	virtual void Dispatch(ExpOver* expOver, int numOfBytes = 0) override;

private:
	void doAccept(AcceptOver* acceptOver);
	void AcceptCallback(AcceptOver* acceptOver);

private:
	SOCKET _socket{ INVALID_SOCKET };
	std::vector<AcceptOver*> _acceptOvers;
	std::weak_ptr<Service> _service;

private:
	std::atomic<bool> _accepting{ false };
};

