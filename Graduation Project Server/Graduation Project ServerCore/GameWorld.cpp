#include "pch.h"
#include "GameWorld.h"

GameWorld::GameWorld(IGameContext& gameCtx) : _gameCtx(gameCtx), _nextInstanceId(0)
{
	AddInstance(InstanceType::Test);
}

void GameWorld::AddInstance(InstanceType type)
{
	std::shared_ptr<Instance> instance;
	switch (type) {
	case InstanceType::Test:
		instance = std::make_shared<TestInstance>(_nextInstanceId, _gameCtx);
		break;
	}

	{
		std::unique_lock lock{ _mutex };
		_instances.try_emplace(_nextInstanceId++, instance);
	}
}

void GameWorld::RemoveInstance(int instanceId)
{
	std::unique_lock lock{ _mutex };

	auto it = _instances.find(instanceId);
	if (it != _instances.end()) {
		_instances.erase(it);
	}
}

Instance* GameWorld::GetInstance(int instanceId)
{
	std::shared_lock lock{ _mutex };

	auto it = _instances.find(instanceId);
	if (it != _instances.end()) {
		return it->second.get();
	}

	return nullptr;
}

void GameWorld::Update(float deltaTime)
{
	for (auto& [id, instance] : _instances) {
		_gameCtx.GetJobQueue().Push(new LogicJob(instance, deltaTime));
	}
}