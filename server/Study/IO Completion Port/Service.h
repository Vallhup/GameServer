#pragma once

#include "Sector.h"
#include "ChatManager.h"

class Listener;

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
	void enterSector(const std::shared_ptr<GameSession>& session);
	void leaveSector(const std::shared_ptr<GameSession>& session);
	std::unordered_set<int> collectVisibleClients(const std::shared_ptr<GameSession>& session) const;

public:
	void OnChatRequest(short senderId, const char* msg);
	void Broadcast(const std::vector<char>& buf);

public:
	int getCurrentSessionCount() const { return _sessionCount; }
	int getMaxSessionCount() const { return _maxSessionCount; }
	std::shared_ptr<IocpCore>& getIocpCore() { return _iocpCore; }

	static std::shared_ptr<Service> Create(std::shared_ptr<IocpCore> core, int maxSessionCount = 10);

private:
	std::shared_ptr<IocpCore> _iocpCore;
	std::shared_ptr<Listener> _listener{ nullptr };

private:
	ChatManager _chatManager;

private:
	// session °ü¸®
	concurrency::concurrent_unordered_map<int, std::atomic<std::shared_ptr<GameSession>>> _sessions;

	std::atomic<int> _sessionCount{ 0 };
	int _maxSessionCount{ 0 };

private:
	std::array<std::array<Sector, SECTOR_COUNT>, SECTOR_COUNT> _sectors;

private:
	std::vector<std::thread> _workers;

public:
	std::atomic<bool> _running{ false };
};

