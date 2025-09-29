#pragma once

class IGameContext {
public:
	virtual ~IGameContext() = default;

public:
	// 1. Network BroadCast
	virtual void BroadCast(const std::vector<char>& packet, int exceptId = -1) = 0;

	// 2. Manager 접근
	virtual class ISessionManager& GetSessionManager() = 0;
	virtual class IDBManager& GetDBManager() = 0;
	virtual class IGameWorld& GetGameWorld() = 0;
	virtual class IocpCore& GetIocpCore() = 0;
	virtual class JobQueue& GetJobQueue() = 0;

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
	virtual IDBManager& GetDBManager() override { return *_dbManager; }
	virtual IGameWorld& GetGameWorld() override { return *_gameWorld; }
	virtual IocpCore& GetIocpCore() override { return *_iocpCore; }
	virtual JobQueue& GetJobQueue() override { return _jobQueue; };

	virtual float GetNowTime() override;

	virtual class ScriptVM& GetScriptVM() override { return *_scriptVM; }

private:
	void TickFunc();
	void IocpFunc();
	void LogicFunc();

private:
	std::atomic<bool> _running;

	std::thread _tickThread;
	ThreadPool _iocpWorker;
	ThreadPool _logicWorker;

	JobQueue _jobQueue;

	std::shared_ptr<Listener> _listener;
	std::unique_ptr<IocpCore> _iocpCore;
	std::unique_ptr<IDBManager> _dbManager;
	std::unique_ptr<ISessionManager> _sessMng;
	std::unique_ptr<IGameWorld> _gameWorld;

	inline static thread_local std::unique_ptr<class ScriptVM> _scriptVM;
};