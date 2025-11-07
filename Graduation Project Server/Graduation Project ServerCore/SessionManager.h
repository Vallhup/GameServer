#pragma once

class SessionManager {
public:
	SessionManager() = delete;
	SessionManager(IGameContext& gameCtx);
	virtual ~SessionManager() = default;

public:
	void AddSession(SOCKET clientSocket);
	void RemoveSession(int sessionId);

	Session* GetSession(int sessionId);
	std::vector<Session*> GetSessionList();

	SendOver* GetSendOver();
	void ReleaseSendOver(SendOver* sendOver);

	void FlushAllSessions();

private:
	void OnSessionPacket(int sessionId, const std::vector<char>& packet);

private:
	IGameContext& _gameCtx;

	mutable std::shared_mutex _mutex;
	std::unordered_map<int, std::shared_ptr<Session>> _sessions;

	std::atomic<int> _nextSessionId;

	inline static thread_local ObjectPool<SendOver, 2000> _sendOverPool;
};