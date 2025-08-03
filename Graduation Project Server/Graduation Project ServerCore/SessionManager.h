#pragma once

class ISessionManager {
public:
	virtual ~ISessionManager() = default;

public:
	virtual void AddSession(SOCKET clientSocket) = 0;;
	virtual void RemoveSession(int sessionId) = 0;
};

class SessionManager : public ISessionManager {
public:
	SessionManager() = delete;
	SessionManager(IGameContext& gameCtx);
	virtual ~SessionManager() = default;

public:
	virtual void AddSession(SOCKET clientSocket) override;
	virtual void RemoveSession(int sessionId) override;

private:
	void OnSessionPacket(int sessionId, const std::vector<char>& packet);

private:
	IGameContext& _gameCtx;

	mutable std::shared_mutex _mutex;
	std::unordered_map<int, std::shared_ptr<Session>> _sessions;

	std::atomic<int> _nextSessionId;
};

