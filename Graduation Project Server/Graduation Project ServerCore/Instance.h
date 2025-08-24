#pragma once

#include "ObjectManager.h"

class TickSystem;
class IGameLogic;

class Instance {
public:
	Instance() = delete;
	Instance(int id, IGameContext& gameCtx) : _id(id), _gameCtx(gameCtx) {}
	virtual ~Instance() = default;

public:
	virtual void Start() = 0;
	virtual void Update(float deltaTime) { _tickSystem.Tick(deltaTime); }
	virtual void Stop() = 0;

protected:
	virtual void LoadStaticGameObject() = 0;

public:
	void AddSession(Session* session);
	void RemoveSession(int sessionId);

	void BroadCast(const std::vector<char>& packet, int exceptId = -1);

	void AddObject(const std::shared_ptr<class GameObject>& obj);
	void RemoveObject(int id);

	int GetId() const { return _id; }
	TickSystem& GetTickSystem() { return _tickSystem; }
	InputSystem& GetInputSystem() { return _inputSystem; }
	IGameLogic& GetGameLogic() const { return *_gameLogic; }
	bool IsActive() const { return _isActive.load(); }
	std::shared_ptr<GameObject> GetGameObject(int id) const { return _objMng->GetGameObject(id); }

protected:
	int _id;
	std::atomic<bool> _isActive;

	IGameContext& _gameCtx;

	TickSystem _tickSystem;
	InputSystem _inputSystem;

	std::unique_ptr<class IGameLogic> _gameLogic;
	std::unique_ptr<class IEventManager> _eventMng;
	std::unique_ptr<class IObjectManager> _objMng;

	mutable std::shared_mutex _mutex;
	std::unordered_map<int, Session*> _sessions;
};