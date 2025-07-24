#pragma once

class Listener;

class ICharacterManager;
class IProjectileManager;
class ICollisionManager;
class ISessionManager;
class ITimerManager;
class IGameLogic;

class IGameContext {
public:
	virtual ~IGameContext() = default;

public:
	// 1. Network BroadCast
	virtual void BroadCast(const std::vector<char>& packet, int exceptId = -1) = 0;

	// 2. 각종 Manager 접근
	virtual ICharacterManager& GetCharacterManager() = 0;
	virtual IProjectileManager& GetProjectileManager() = 0;
	virtual ICollisionManager& GetCollisionManager() = 0;
	virtual ISessionManager& GetSessionManager() = 0;
	virtual ITimerManager& GetTimerManager() = 0;
	virtual IGameLogic& GetGameLogic() = 0;

	// 3. 시간 정보
	virtual float GetNowTime() = 0;
};

class Service 
	: public std::enable_shared_from_this<Service>, public IGameContext {
public:
	Service();
	virtual ~Service();

	Service(const Service&) = delete;
	Service& operator=(const Service&) = delete;

	Service(Service&&) = delete;
	Service& operator=(Service&&) = delete;

public:
	bool Init();
	void Start();
	void Stop();

	virtual void BroadCast(const std::vector<char>& packet, int exceptId = -1) override;

	virtual ICharacterManager& GetCharacterManager() override { return *_charMng; }
	virtual IProjectileManager& GetProjectileManager() override { return *_projMng; }
	virtual ICollisionManager& GetCollisionManager() override { return *_collMng; }
	virtual ISessionManager& GetSessionManager() override { return *_sessMng; }
	virtual ITimerManager& GetTimerManager() override { return *_timerMng; }
	virtual IGameLogic& GetGameLogic() override { return *_gameLogic; }

	virtual float GetNowTime() override;

private:
	bool _running{ false };

	std::unique_ptr<Listener> _listener;

	std::unique_ptr<ICharacterManager> _charMng;
	std::unique_ptr<IProjectileManager> _projMng;
	std::unique_ptr<ICollisionManager> _collMng;
	std::unique_ptr<ISessionManager> _sessMng;
	std::unique_ptr<ITimerManager> _timerMng;
	std::unique_ptr<IGameLogic> _gameLogic;
};

