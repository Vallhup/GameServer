#pragma once

class IocpObject
{
public:
	virtual HANDLE GetHandle() abstract;
	virtual void Dispatch(class ExpOver* expOver, int nuOfBytes = 0) abstract;

public:
	void SetId(int sessionId) { _sessionId = sessionId; }

protected:
	int _sessionId{ 0 };
};

class Service;

class IocpCore
{
public:
	IocpCore();
	~IocpCore();

public:
	HANDLE GetHandle() { return _iocpHandle; };

	void SetService(std::shared_ptr<Service> service) { _service = service; }

public:
	bool Register(std::shared_ptr<IocpObject> iocpObject);
	bool Dispatch(unsigned int timeoutMs = INFINITE);

private:
	HANDLE _iocpHandle;
	std::weak_ptr<Service> _service;

	static int clientId;
};

