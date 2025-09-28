#pragma once

class ISessionManager {
public:
	virtual ~ISessionManager() = default;

public:
	virtual void AddSession(SOCKET clientSocket) = 0;;
	virtual void RemoveSession(int sessionId) = 0;

	virtual Session* GetSession(int sessionId) = 0;
	virtual std::vector<Session*> GetSessionList() = 0;

	virtual void SetCharacter(int sessionId, GameObject* character) = 0;

	virtual SendOver* GetSendOver() = 0;
	virtual void ReleaseSendOver(SendOver* sendOver) = 0;
};

class SessionManager : public ISessionManager {
public:
	SessionManager() = delete;
	SessionManager(IGameContext& gameCtx);
	virtual ~SessionManager() = default;

public:
	virtual void AddSession(SOCKET clientSocket) override;
	virtual void RemoveSession(int sessionId) override;

	virtual Session* GetSession(int sessionId) override;
	virtual std::vector<Session*> GetSessionList() override;

	virtual void SetCharacter(int sessionId, GameObject* character) override;

	virtual SendOver* GetSendOver() override;
	virtual void ReleaseSendOver(SendOver* sendOver) override;

private:
	void OnSessionPacket(int sessionId, const std::vector<char>& packet);

private:
	IGameContext& _gameCtx;

	mutable std::shared_mutex _mutex;
	std::unordered_map<int, std::shared_ptr<Session>> _sessions;

	std::atomic<int> _nextSessionId;

	inline static thread_local ObjectPool<SendOver, 2000> _sendOverPool;
};