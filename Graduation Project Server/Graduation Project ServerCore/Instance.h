#pragma once

#include "ObjectManager.h"

class TickSystem;
class IGameLogic;

enum class InstanceType : char {
	Town,
	Main,
	Boss,
	Pvp,
	Test
};

class Instance {
public:
	Instance() = delete;
	Instance(int id, InstanceType type, IGameContext& gameCtx);
	virtual ~Instance() = default;

public:
	virtual void Start() = 0;
	virtual void Update(float deltaTime);
	virtual void Stop() = 0;

protected:
	virtual void LoadStaticGameObject() = 0;

public:
	void EnqueueJob(const std::function<void()>& job);

	void AddPlayer(Session* session);
	void RemovePlayer(int sessionId);

	void BroadCast(const std::vector<char>& packet, int exceptId = -1);

	void AddObject(const std::shared_ptr<class GameObject>& obj);
	void RemoveObject(int id);

	int GetId() const { return _id; }
	IGameLogic& GetGameLogic() const { return *_gameLogic; }
	bool IsActive() const { return _isActive.load(); }
	std::shared_ptr<GameObject> GetGameObject(int id) const { return _objMng->GetGameObject(id); }
	std::vector<std::shared_ptr<GameObject>> GetGameObjectList() const;

protected:
	void DequeueJobs();

protected:
	int _id;
	std::atomic<bool> _isActive;

	IGameContext& _gameCtx;;

	std::unique_ptr<class IGameLogic> _gameLogic;
	std::unique_ptr<class IObjectManager> _objMng;

	mutable std::shared_mutex _mutex;
	std::unordered_map<int, Session*> _sessions;

	std::atomic<bool> _isUpdating;

	JobQueue _jobQueue;

	InstanceType _type;
};