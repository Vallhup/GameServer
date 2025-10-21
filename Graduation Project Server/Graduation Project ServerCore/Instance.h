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

	void AddObject(std::unique_ptr<class GameObject> obj);
	void RemoveObject(int id);

	int GetId() const { return _id; }
	IGameLogic& GetGameLogic() const { return *_gameLogic; }
	bool IsActive() const { return _isActive.load(); }
	GameObject* GetGameObject(int id) const { return _objMng->GetGameObject(id); }
	std::vector<GameObject*> GetGameObjectList() const;

public:
	JobQueue& GetJobQueue() { return _gameCtx.GetJobQueue(); }

protected:
	void DequeueJobs();

protected:
	int _id;
	InstanceType _type;
	std::atomic<bool> _isActive;
	std::atomic<bool> _isUpdating;

	IGameContext& _gameCtx;;

	JobQueue _jobQueue;
	std::unordered_set<int> _sessions;
	std::unique_ptr<class IGameLogic> _gameLogic;
	std::unique_ptr<class IObjectManager> _objMng;
};