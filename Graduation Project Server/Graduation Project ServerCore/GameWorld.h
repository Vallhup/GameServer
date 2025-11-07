#pragma once

class GameWorld {
public:
	GameWorld() = delete;
	GameWorld(IGameContext& gameCtx);
	virtual ~GameWorld() = default;

public:
	void AddInstance(InstanceType type);
	void RemoveInstance(int instanceId);

	Instance* GetInstance(int instanceId);

	void Update(float deltaTime);

private:
	IGameContext& _gameCtx;

	std::atomic<int> _nextInstanceId;

	mutable std::shared_mutex _mutex;
	std::unordered_map<int, std::shared_ptr<class Instance>> _instances;
};