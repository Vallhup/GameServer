#include "pch.h"
#include "CharacterSpawnService.h"

#include <algorithm>

#include "Aspect/CharacterAspectRegistry.h"
#include "Aspect/ICharacterAspect.h"
#include "FrameworkRuntime.h"
#include "WorldInstance.h"

namespace
{
	CharacterSpawnResult MakeResult(
		CharacterSpawnResultCode code,
		SessionId sessionId,
		CharacterId characterId = CharacterId::None,
		WorldId worldId = WorldId::Invalid(),
		Entity entity = Entity::Null(),
		NetId netId = NetId::Invalid()) noexcept
	{
		CharacterSpawnResult result{};
		result.code = code;
		result.sessionId = sessionId;
		result.characterId = characterId;
		result.worldId = worldId;
		result.entity = entity;
		result.netId = netId;
		return result;
	}
}

CharacterSpawnService::CharacterSpawnService(Dependencies deps)
	: _deps(deps)
{
}

void CharacterSpawnService::SetDependencies(Dependencies deps) noexcept
{
	_deps = deps;
}

void CharacterSpawnService::Clear() noexcept
{
	_pendingSpawns.clear();
}

CharacterSpawnResult CharacterSpawnService::RequestCharacterSpawn(
	const CharacterDataResult& data,
	NetId reservedPlayerNetId)
{
	if (data.sessionId == 0)
	{
		return MakeResult(CharacterSpawnResultCode::InvalidSession, data.sessionId, data.characterId);
	}

	if (!data.Succeeded() ||
		data.characterDef == nullptr ||
		!data.worldId.IsValid())
	{
		return MakeResult(
			CharacterSpawnResultCode::InvalidCharacterData,
			data.sessionId,
			data.characterId,
			data.worldId);
	}

	if (_deps.framework == nullptr ||
		!reservedPlayerNetId.IsValid() ||
		!_deps.framework->IsNetIdAlive(reservedPlayerNetId))
	{
		return MakeResult(
			CharacterSpawnResultCode::InvalidReservedNetId,
			data.sessionId,
			data.characterId,
			data.worldId,
			Entity::Null(),
			reservedPlayerNetId);
	}

	if (HasPendingSpawn(data.sessionId))
	{
		return MakeResult(
			CharacterSpawnResultCode::DuplicatePendingSpawn,
			data.sessionId,
			data.characterId,
			data.worldId,
			Entity::Null(),
			reservedPlayerNetId);
	}

	WorldInstance* const world = _deps.framework->FindWorld(data.worldId);
	if (world == nullptr)
	{
		return MakeResult(
			CharacterSpawnResultCode::WorldNotFound,
			data.sessionId,
			data.characterId,
			data.worldId,
			Entity::Null(),
			reservedPlayerNetId);
	}

	WorldRuntime& runtime = world->GetRuntime();
	const Entity playerEntity = runtime.ReserveEntity();
	if (playerEntity.IsNull())
	{
		return MakeResult(
			CharacterSpawnResultCode::EntityReserveFailed,
			data.sessionId,
			data.characterId,
			data.worldId,
			Entity::Null(),
			reservedPlayerNetId);
	}

	if (!_deps.framework->BindNetEntity(reservedPlayerNetId, data.worldId, playerEntity))
	{
		runtime.DeferredDestroyEntity(playerEntity);
		return MakeResult(
			CharacterSpawnResultCode::NetBindFailed,
			data.sessionId,
			data.characterId,
			data.worldId,
			playerEntity,
			reservedPlayerNetId);
	}

	AssembleParams params{};
	params.position = data.spawnPosition;
	params.rotation = data.spawnRotation;
	params.netId = reservedPlayerNetId;
	params.sessionId = data.sessionId;

	GetGlobalCharacterAspectRegistry().Assemble(
		runtime,
		playerEntity,
		*data.characterDef,
		params);

	_pendingSpawns.push_back(
		PendingSessionCharacterSpawn
		{
			.sessionId = data.sessionId,
			.worldId = data.worldId,
			.entity = playerEntity,
			.characterId = data.characterId,
			.netId = reservedPlayerNetId
		});

	return MakeResult(
		CharacterSpawnResultCode::Success,
		data.sessionId,
		data.characterId,
		data.worldId,
		playerEntity,
		reservedPlayerNetId);
}

bool CharacterSpawnService::CancelPendingSpawn(SessionId sessionId) noexcept
{
	if (sessionId == 0)
	{
		return false;
	}

	bool canceled = false;
	for (const PendingSessionCharacterSpawn& pending : _pendingSpawns)
	{
		if (pending.sessionId != sessionId ||
			!pending.worldId.IsValid() ||
			pending.entity.IsNull() ||
			_deps.framework == nullptr)
		{
			continue;
		}

		if (WorldInstance* const world = _deps.framework->FindWorld(pending.worldId))
		{
			world->GetRuntime().DeferredDestroyEntity(pending.entity);
			(void)_deps.framework->UnbindNetEntity(pending.netId);
			canceled = true;
		}
	}

	const auto newEnd = std::remove_if(
		_pendingSpawns.begin(),
		_pendingSpawns.end(),
		[sessionId](const PendingSessionCharacterSpawn& pending)
		{
			return pending.sessionId == sessionId;
		});
	const bool erasedPending = newEnd != _pendingSpawns.end();
	_pendingSpawns.erase(newEnd, _pendingSpawns.end());

	return canceled || erasedPending;
}

const PendingSessionCharacterSpawn* CharacterSpawnService::FindPendingSpawn(
	SessionId sessionId) const noexcept
{
	const auto it = std::find_if(
		_pendingSpawns.begin(),
		_pendingSpawns.end(),
		[sessionId](const PendingSessionCharacterSpawn& pending)
		{
			return pending.sessionId == sessionId;
		});

	return it != _pendingSpawns.end() ? &(*it) : nullptr;
}

const PendingSessionCharacterSpawn* CharacterSpawnService::FindPendingSpawn(
	WorldId worldId,
	Entity entity) const noexcept
{
	const auto it = std::find_if(
		_pendingSpawns.begin(),
		_pendingSpawns.end(),
		[worldId, entity](const PendingSessionCharacterSpawn& pending)
		{
			return pending.worldId == worldId && pending.entity == entity;
		});

	return it != _pendingSpawns.end() ? &(*it) : nullptr;
}

std::span<const PendingSessionCharacterSpawn> CharacterSpawnService::GetPendingSpawns() const noexcept
{
	return std::span<const PendingSessionCharacterSpawn>(_pendingSpawns);
}

bool CharacterSpawnService::TryConsumeSpawnConfirmed(
	WorldId worldId,
	Entity entity,
	PendingSessionCharacterSpawn& outPendingSpawn)
{
	const auto it = std::find_if(
		_pendingSpawns.begin(),
		_pendingSpawns.end(),
		[worldId, entity](const PendingSessionCharacterSpawn& pending)
		{
			return pending.worldId == worldId && pending.entity == entity;
		});

	if (it == _pendingSpawns.end())
	{
		return false;
	}

	outPendingSpawn = *it;
	_pendingSpawns.erase(it);
	return true;
}

bool CharacterSpawnService::HasPendingSpawn(SessionId sessionId) const noexcept
{
	return FindPendingSpawn(sessionId) != nullptr;
}
