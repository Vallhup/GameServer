#pragma once

class IGameWorld {
public:
	virtual ~IGameWorld() = default;

public:
	virtual void AddInstance(InstanceType type) = 0;
	virtual void RemoveInstance(int instanceId) = 0;

	virtual Instance* GetInstance(int instanceId) = 0;

	virtual void Update(float deltaTime) = 0;
};

class GameWorld : public IGameWorld {
public:
	GameWorld() = delete;
	GameWorld(IGameContext& gameCtx);
	virtual ~GameWorld() = default;

public:
	virtual void AddInstance(InstanceType type) override;
	virtual void RemoveInstance(int instanceId) override;

	virtual Instance* GetInstance(int instanceId) override;

	virtual void Update(float deltaTime) override;

private:
	IGameContext& _gameCtx;

	std::atomic<int> _nextInstanceId;

	mutable std::shared_mutex _mutex;
	std::unordered_map<int, std::shared_ptr<class Instance>> _instances;
};