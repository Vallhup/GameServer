#pragma once

class IGameWorld {
public:
	virtual ~IGameWorld() = default;

public:
	virtual Instance* GetInstance(int instanceId) = 0;
};

class GameWorld : public IGameWorld {
public:
	GameWorld() = default;
	virtual ~GameWorld() = default;

public:
	virtual Instance* GetInstance(int instanceId) override
	{
		auto it = _instances.find(instanceId);
		if (it != _instances.end()) {
			return it->second.get();
		}

		return nullptr;
	}

private:
	mutable std::shared_mutex _mutex;
	std::unordered_map<int, std::shared_ptr<class Instance>> _instances;
};