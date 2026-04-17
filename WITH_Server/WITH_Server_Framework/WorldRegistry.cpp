#include "pch.h"
#include "WorldRegistry.h"

WorldRegistry::WorldRegistry(
	IWorldInstanceFactory& factory,
	const IWorldTransferBinding* transferBinding)
	: _factory(factory)
	, _transferBinding(transferBinding)
{
}

WorldRegistry::WorldRegistry(
	IWorldInstanceFactory& factory,
	WorldExecutionModelRegistry& executionModelRegistry,
	const IWorldTransferBinding* transferBinding)
	: _factory(factory)
	, _executionModelRegistry(&executionModelRegistry)
	, _transferBinding(transferBinding)
{
}

WorldRegistry::WorldRegistry(
	IWorldInstanceFactory& factory,
	WorldExecutionModelRegistry& executionModelRegistry,
	WorldTransferProfileRegistry& transferProfileRegistry,
	const IWorldTransferBinding* transferBinding)
	: _factory(factory)
	, _executionModelRegistry(&executionModelRegistry)
	, _transferProfileRegistry(&transferProfileRegistry)
	, _transferBinding(transferBinding)
{
}

WorldRegistry::WorldRegistry(
	IWorldInstanceFactory& factory,
	WorldTransferProfileRegistry& transferProfileRegistry,
	const IWorldTransferBinding* transferBinding)
	: _factory(factory)
	, _transferProfileRegistry(&transferProfileRegistry)
	, _transferBinding(transferBinding)
{
}

const WorldDef* WorldRegistry::FindWorldDef(WorldDefId defId) const
{
	auto it = _defs.find(defId);
	if (it == _defs.end())
		return nullptr;

	return &it->second;
}

bool WorldRegistry::RegisterWorldDef(const WorldDef& def)
{
	if (def.id == WorldDefId::None)
		return false;

	if (def.executionModelKey == InvalidWorldExecutionModelKey)
		return false;

	if (_executionModelRegistry == nullptr)
		return false;

	if (!_executionModelRegistry->Has(def.executionModelKey))
		return false;

	if (def.transferProfileId != InvalidWorldTransferProfileId)
	{
		if (_transferProfileRegistry == nullptr)
			return false;

		if (!_transferProfileRegistry->Has(def.transferProfileId))
			return false;
	}

	_defs[def.id] = def;
	return true;
}

WorldInstance* WorldRegistry::FindWorld(WorldId worldId)
{
	auto it = _worlds.find(worldId);
	if (it == _worlds.end())
		return nullptr;

	return it->second.get();
}

const WorldInstance* WorldRegistry::FindWorld(WorldId worldId) const
{
	auto it = _worlds.find(worldId);
	if (it == _worlds.end())
		return nullptr;

	return it->second.get();
}

WorldInstance* WorldRegistry::CreateWorld(const WorldDef& def, uint64_t instanceKey)
{
	auto impl = _factory.Create(def);
	if (!impl)
		return nullptr;

	if (def.executionModelKey == InvalidWorldExecutionModelKey)
		return nullptr;

	if (_executionModelRegistry == nullptr)
		return nullptr;

	const WorldExecutionModel* executionModel =
		_executionModelRegistry->TryGet(def.executionModelKey);
	if (executionModel == nullptr)
		return nullptr;

	const WorldTransferProfile* transferProfile = nullptr;
	if (def.transferProfileId != InvalidWorldTransferProfileId)
	{
		if (_transferProfileRegistry == nullptr)
			return nullptr;

		transferProfile = _transferProfileRegistry->Find(def.transferProfileId);
		if (transferProfile == nullptr)
			return nullptr;
	}

	const WorldId worldId = _idAllocator.Allocate();
	if (!worldId.IsValid())
		return nullptr;

	WorldInstanceCreateParams params;
	params.identity.id = worldId;
	params.identity.defId = def.id;
	params.identity.instanceKey = instanceKey;
	params.def = &def;
	params.executionModel = *executionModel;
	params.transferProfile = transferProfile;
	params.transferBinding = _transferBinding;
	params.impl = std::move(impl);

	auto instance = std::make_unique<WorldInstance>(std::move(params));
	WorldInstance* raw = instance.get();

	_worlds.try_emplace(worldId, std::move(instance));
	return raw;
}

bool WorldRegistry::DestroyWorld(WorldId worldId)
{
	auto it = _worlds.find(worldId);
	if (it == _worlds.end())
		return false;

	it->second->Shutdown();
	_worlds.erase(it);

	_idAllocator.Free(worldId);
	return true;
}

bool WorldRegistry::IsAlive(WorldId worldId) const
{
	return _idAllocator.IsAlive(worldId);
}

void WorldRegistry::Clear()
{
	for (auto& [_, world] : _worlds)
	{
		if (world)
			world->Shutdown();
	}

	_worlds.clear();
	_defs.clear();
	_idAllocator.Clear();
}

