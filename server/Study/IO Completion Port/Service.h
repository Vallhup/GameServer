#pragma once

#include "Listener.h"

class Service : public std::enable_shared_from_this<Service>
{
	friend class Session;
	friend class GameSession;

public:
	Service(std::shared_ptr<IocpCore> core, int maxSessionCount = 10);

public:
	bool Start();
	void CloseService();

	std::shared_ptr<Session> CreateSession();
	void AddSession(const std::shared_ptr<GameSession> session);
	void ReleaseSession(const std::shared_ptr<GameSession> session);

public:
	int getCurrentSessionCount() const { return _sessionCount; }
	int getMaxSessionCount() const { return _maxSessionCount; }
	std::shared_ptr<IocpCore>& getIocpCore() { return _iocpCore; }

	static std::shared_ptr<Service> Create(std::shared_ptr<IocpCore> core, int maxSessionCount = 10);

private:
	std::shared_ptr<IocpCore> _iocpCore;
	std::shared_ptr<Listener> _listener{ nullptr };

private:
	// session °ü¸®
	concurrency::concurrent_unordered_map<int, std::atomic<std::shared_ptr<GameSession>>> _sessions;

	std::atomic<int> _sessionCount{ 0 };
	int _maxSessionCount{ 0 };

private:
	std::vector<std::thread> _workers;

public:
	std::atomic<bool> _running{ false };

};

