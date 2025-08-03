#pragma once

class Instance {
public:
	Instance() = delete;
	Instance(IGameContext& gameCtx) : _gameCtx(gameCtx) {}
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

	void AddObject(const std::shared_ptr<class DynamicGameObject>& obj);
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

class TownInstance : public Instance {
public:
	TownInstance() = delete;
	TownInstance(IGameContext& gameCtx) : Instance(gameCtx) { LoadStaticGameObject(); }
	virtual ~TownInstance() = default;

public:
	virtual void Update(float deltaTime) override;
	virtual void Stop() override;

private:
	virtual void LoadStaticGameObject() override;
};

class MainInstance : public Instance {
public:
	MainInstance() = delete;
	MainInstance(IGameContext& gameCtx) : Instance(gameCtx) { LoadStaticGameObject(); }
	virtual ~MainInstance() = default;

public:
	virtual void Update(float deltaTime) override;
	virtual void Stop() override;

private:
	virtual void LoadStaticGameObject() override;
};

class BossInstance : public Instance {
public:
	BossInstance() = delete;
	BossInstance(IGameContext& gameCtx) : Instance(gameCtx) { LoadStaticGameObject(); }
	virtual ~BossInstance() = default;

public:
	virtual void Update(float deltaTime) override;
	virtual void Stop() override;

private:
	virtual void LoadStaticGameObject() override;
};

class PvpInstnace : public Instance {
public:
	PvpInstnace() = delete;
	PvpInstnace(IGameContext& gameCtx) : Instance(gameCtx) { LoadStaticGameObject(); }
	virtual ~PvpInstnace() = default;

public:
	virtual void Update(float deltaTime) override;
	virtual void Stop() override;

private:
	virtual void LoadStaticGameObject() override;
};
