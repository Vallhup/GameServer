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
	case InstanceType::Town:
		instance = std::make_shared<TownInstance>(_nextInstanceId, _gameCtx);
		break;
	case InstanceType::Main:
		instance = std::make_shared<MainInstance>(_nextInstanceId, _gameCtx);
		break;
	case InstanceType::Boss:
		instance = std::make_shared<BossInstance>(_nextInstanceId, _gameCtx);
		break;
	case InstanceType::Pvp:
		instance = std::make_shared<PvpInstance>(_nextInstanceId, _gameCtx);
		break;
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
	std::vector<std::shared_ptr<Instance>> snap;
	{
		std::shared_lock lock{ _mutex };
		for (auto& [id, instance] : _instances) {
			if (instance) {
				snap.push_back(instance);
			}
		}
	}

	for (auto& instance : snap) {
		_gameCtx.GetJobQueue().Push(std::make_shared<LogicJob>(instance, deltaTime));
	}
}