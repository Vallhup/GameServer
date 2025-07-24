#pragma once

class ISessionManager {
public:
	virtual ~ISessionManager() = default;

public:
	virtual void AcceptSession(SOCKET clientSocket) = 0;
	virtual void CloseSession(int sessionId) = 0;

	virtual void AddSession(const std::shared_ptr<Session>& session) = 0;
	virtual void RemoveSession(int sessionId) = 0;

	virtual std::shared_ptr<Session> GetSession(int sessionId) const = 0;
	virtual std::vector<std::shared_ptr<Session>> GetSessionList() const = 0;
};

class SessionManager : public ISessionManager {
	static constexpr short MAX_SESSION{ 64 };

public:
	SessionManager() = delete;
	SessionManager(IGameContext& gameCtx);
	virtual ~SessionManager() = default;

	SessionManager(const SessionManager&) = delete;
	SessionManager& operator=(const SessionManager&) = delete;

	SessionManager(SessionManager&&) = delete;
	SessionManager& operator=(SessionManager&&) = delete;

public:
	virtual void AcceptSession(SOCKET clientSocket) override;
	virtual void CloseSession(int sessionId) override;

	virtual void AddSession(const std::shared_ptr<Session>& session) override;
	virtual void RemoveSession(int sessionId) override;

	virtual std::shared_ptr<Session> GetSession(int sessionId) const override;
	virtual std::vector<std::shared_ptr<Session>> GetSessionList() const override;

private:
	int GenerateSessionId();

	void OnSessionPacket(int sessionId, const std::vector<char>& packet);

private:
	IGameContext& _gameCtx;

	std::vector<int> _sessionIds;

	mutable std::shared_mutex _mutex;
	std::unordered_map<int, std::shared_ptr<Session>> _sessions;
};