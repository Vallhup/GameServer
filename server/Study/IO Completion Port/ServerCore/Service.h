#pragma once

class Service : public std::enable_shared_from_this<Service>
{
public:
	Service(std::shared_ptr<IocpCore> core, int maxSessionCount = 10);
	virtual ~Service();

public:
	virtual bool Start() abstract;
	virtual void CloseService() abstract;

	std::shared_ptr<Session> CreateSession();
	void AddSession(std::shared_ptr<Session> session);
	void ReleaseSession(std::shared_ptr<Session> session);

public:
	int getCurrentSessionCount() { return _sessionCount; }
	int getMaxSessionCount() { return _maxSessionCount; }
	std::shared_ptr<IocpCore>& getIocpCore() { return _iocpCore; }

protected:
	std::shared_ptr<IocpCore> _iocpCore;

	std::unordered_map<int, std::shared_ptr<Session>> _sessions;

	int _sessionCount{ 0 };
	int _maxSessionCount{ 0 };
};

class ServerService : public Service
{
public:
	ServerService(std::shared_ptr<IocpCore> core, int maxSessionCount = 10);
	virtual ~ServerService() {}

public:
	virtual bool Start() override;
	virtual void CloseService() override;

private:
	std::shared_ptr<Listener> _listener{ nullptr };
};

