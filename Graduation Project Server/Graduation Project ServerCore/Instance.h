#pragma once

class Instance {
public:
	Instance() = delete;
	Instance(int id, IGameContext& gameCtx) : _id(id), _gameCtx(gameCtx) {}
	virtual ~Instance() = default;

public:
	virtual void Update(float deltaTime) = 0;
	virtual void Stop() = 0;

private:
	virtual void LoadStaticGameObject() = 0;

public:
	void AddSession(Session* session);
	void RemoveSession(int sessionId);

	void BroadCast(const std::vector<char>& packet, int exceptId = -1);

	void AddObject(const std::shared_ptr<class GameObject>& obj);
	void RemoveObject(const ObjectId& id);

	bool GetId() const { return _id; }
	bool IsActive() const { return _isActive.load(); }

protected:
	int _id;
	std::atomic<bool> _isActive;

	IGameContext& _gameCtx;

	std::unique_ptr<class IObjectManager> _objMng;

	mutable std::shared_mutex _mutex;
	std::unordered_map<int, Session*> _sessions;
};