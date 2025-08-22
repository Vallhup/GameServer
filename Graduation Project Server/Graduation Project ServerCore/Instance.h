#pragma once

class TickSystem;

class Instance {
public:
	Instance() = delete;
	Instance(int id, IGameContext& gameCtx) : _id(id), _gameCtx(gameCtx) {}
	virtual ~Instance() = default;

public:
	virtual void Start() = 0;
	virtual void Update(float deltaTime) { _scheduler.Tick(deltaTime); }
	virtual void Stop() = 0;

protected:
	virtual void LoadStaticGameObject() = 0;

public:
	void AddSession(Session* session);
	void RemoveSession(int sessionId);

	void BroadCast(const std::vector<char>& packet, int exceptId = -1);

	void AddObject(const std::shared_ptr<class GameObject>& obj);
	void RemoveObject(const ObjectId& id);

	int GetId() const { return _id; }
	TickSystem GetScheduler() const { return _scheduler; }
	bool IsActive() const { return _isActive.load(); }

protected:
	int _id;
	std::atomic<bool> _isActive;

	IGameContext& _gameCtx;
	TickSystem _scheduler;

	std::unique_ptr<class IObjectManager> _objMng;

	mutable std::shared_mutex _mutex;
	std::unordered_map<int, Session*> _sessions;
};