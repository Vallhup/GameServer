#pragma once

#include <vector>
#include <memory>

#include "ThreadPool.h"
#include "GameWorld.h"
#include "Instance.h"
#include "ECS.h"

#include "Constants.h"

class Game {
public:
	Game(size_t size);

	void Update(const float dT);

	ECS ecs;
	JobGraph graph;
	ThreadPool threadPool;
	GameWorld world;

private:
	float _deltaTime{ 0.0f };
};