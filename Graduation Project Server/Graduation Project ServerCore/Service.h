#pragma once

class IGameContext {
public:
	virtual ~IGameContext() = default;

public:
	// 1. Network BroadCast
	virtual void BroadCast(const std::vector<char>& packet, int exceptId = -1) = 0;

	// 2. Manager 접근
	virtual class ISessionManager& GetSessionManager() = 0;
	virtual class IEventManager& GetEventManager() = 0;
	virtual class IGameLogic& GetGameLogic() = 0;
	virtual class IGameWorld& GetGameWorld() = 0;
	virtual class IocpCore& GetIocpCore() = 0;

	// 3. 시간 정보
	virtual float GetNowTime() = 0;

	// 4. Script VM 접근
	virtual class ScriptVM& GetScriptVM() = 0;
};

class Service : public IGameContext {
public:
	Service();
	virtual ~Service() = default;

	Service(const Service&) = delete;
	Service& operator=(const Service&) = delete;

	Service(Service&&) = delete;
	Service& operator=(Service&&) = delete;

public:
	bool Init();
	bool Start();
	void Stop();

	virtual void BroadCast(const std::vector<char>& packet, int exceptId = -1) override;

	virtual ISessionManager& GetSessionManager() override { return *_sessMng; }
	virtual IEventManager& GetEventManager() override { return *_eventMng; }
	virtual IGameLogic& GetGameLogic() override { return *_gameLogic; }
	virtual IGameWorld& GetGameWorld() override { return *_gameWorld; }
	virtual IocpCore& GetIocpCore() override { return *_iocpCore; }

	virtual float GetNowTime() override;

	virtual class ScriptVM& GetScriptVM() override { return *_scriptVM; }

private:
	std::atomic<bool> _running{ false };

	std::vector<std::thread> _workers;

	std::shared_ptr<Listener> _listener;
	std::unique_ptr<IocpCore> _iocpCore;

	std::unique_ptr<ISessionManager> _sessMng;
	std::unique_ptr<IEventManager> _eventMng;
	std::unique_ptr<IGameLogic> _gameLogic;
	std::unique_ptr<IGameWorld> _gameWorld;

	static thread_local std::unique_ptr<ScriptVM> _scriptVM;
};