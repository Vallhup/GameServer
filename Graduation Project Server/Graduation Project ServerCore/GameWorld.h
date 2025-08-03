#pragma once

class IGameWorld {
public:
	virtual ~IGameWorld() = default;

public:
};

class GameWorld : public IGameWorld {
public:
	GameWorld() = default;
	virtual ~GameWorld() = default;

private:
	mutable std::shared_mutex _mutex;
	std::unordered_map<int, std::shared_ptr<Instance>> _instances;
};

