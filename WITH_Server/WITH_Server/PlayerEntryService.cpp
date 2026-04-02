#include "pch.h"
#include "PlayerEntryService.h"

#include <algorithm>

#include "CharacterIdPolicy.h"
#include "RepComponent.h"
#include "WorldInstance.h"

namespace
{
	PlayerEntryResult MakeResult(
		PlayerEntryResultCode code,
		SessionId sessionId,
		CharacterId characterId = CharacterId::None,
		WorldId worldId = WorldId::Invalid(),
		Entity entity = Entity::Null()) noexcept
	{
		PlayerEntryResult result{};
		result.code = code;
		result.sessionId = sessionId;
		result.characterId = characterId;
		result.worldId = worldId;
		result.entity = entity;
		return result;
	}
}

PlayerEntryService::PlayerEntryService(Dependencies deps)
	: _deps(deps)
{
}

void PlayerEntryService::SetDependencies(Dependencies deps) noexcept
{
	_deps = deps;
}

void PlayerEntryService::Clear() noexcept
{
	_contexts.clear();
	_pendingSpawns.clear();
}

bool PlayerEntryService::BeginAuthenticatedEntry(SessionId sessionId)
{
	if (sessionId == 0)
	{
		return false;
	}

	(void)CancelEntry(sessionId);

	PlayerEntryContext* const context = EnsureContext(sessionId);
	if (context == nullptr)
	{
		return false;
	}

	context->stage = EntryFlowStage::AwaitingCharacterSelect;
	context->selectedCharacterId.reset();
	return true;
}

bool PlayerEntryService::CancelEntry(SessionId sessionId) noexcept
{
	if (sessionId == 0)
	{
		return false;
	}

	for (const PendingCharacterSpawn& pending : _pendingSpawns)
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
		}
	}

	const size_t erasedContextCount = _contexts.erase(sessionId);
	const auto newEnd = std::remove_if(
		_pendingSpawns.begin(),
		_pendingSpawns.end(),
		[sessionId](const PendingCharacterSpawn& pending)
		{
			return pending.sessionId == sessionId;
		});
	const bool erasedPending = newEnd != _pendingSpawns.end();
	_pendingSpawns.erase(newEnd, _pendingSpawns.end());

	return erasedContextCount != 0 || erasedPending;
}

const PlayerEntryContext* PlayerEntryService::FindContext(SessionId sessionId) const noexcept
{
	const auto it = _contexts.find(sessionId);
	return it != _contexts.end() ? &it->second : nullptr;
}

PlayerEntryContext* PlayerEntryService::FindContext(SessionId sessionId) noexcept
{
	const auto it = _contexts.find(sessionId);
	return it != _contexts.end() ? &it->second : nullptr;
}

EntryFlowStage PlayerEntryService::FindStage(SessionId sessionId) const noexcept
{
	const PlayerEntryContext* const context = FindContext(sessionId);
	return context != nullptr ? context->stage : EntryFlowStage::None;
}

bool PlayerEntryService::IsAwaitingCharacterSelect(SessionId sessionId) const noexcept
{
	return FindStage(sessionId) == EntryFlowStage::AwaitingCharacterSelect;
}

bool PlayerEntryService::IsSpawnPending(SessionId sessionId) const noexcept
{
	return FindStage(sessionId) == EntryFlowStage::SpawnPending;
}

PlayerEntryResult PlayerEntryService::RequestCharacterSelect(
	SessionId sessionId,
	CharacterId characterId)
{
	if (sessionId == 0)
	{
		return MakeResult(PlayerEntryResultCode::InvalidSession, sessionId, characterId);
	}

	PlayerEntryContext* const context = FindContext(sessionId);
	if (context == nullptr)
	{
		return MakeResult(PlayerEntryResultCode::EntryNotStarted, sessionId, characterId);
	}

	if (context->stage != EntryFlowStage::AwaitingCharacterSelect)
	{
		return MakeResult(PlayerEntryResultCode::InvalidFlowStage, sessionId, characterId);
	}

	if (HasPendingSpawn(sessionId))
	{
		return MakeResult(PlayerEntryResultCode::DuplicatePendingSpawn, sessionId, characterId);
	}

	if (!IsPlayableCharacterId(characterId))
	{
		return MakeResult(PlayerEntryResultCode::CharacterIdNotPlayable, sessionId, characterId);
	}

	const CharacterDef* const characterDef = FindCharacterDef(characterId);
	if (characterDef == nullptr)
	{
		return MakeResult(PlayerEntryResultCode::CharacterDefNotFound, sessionId, characterId);
	}

	if (_deps.framework == nullptr ||
		_deps.startupWorldId == nullptr ||
		!_deps.startupWorldId->IsValid())
	{
		return MakeResult(PlayerEntryResultCode::StartupWorldUnavailable, sessionId, characterId);
	}

	const WorldId startupWorldId = *_deps.startupWorldId;
	WorldInstance* const world = _deps.framework->FindWorld(startupWorldId);
	if (world == nullptr)
	{
		return MakeResult(PlayerEntryResultCode::WorldNotFound, sessionId, characterId, startupWorldId);
	}

	WorldRuntime& runtime = world->GetRuntime();
	const Entity playerEntity = runtime.ReserveEntity();
	if (playerEntity.IsNull())
	{
		return MakeResult(
			PlayerEntryResultCode::EntityReserveFailed,
			sessionId,
			characterId,
			startupWorldId);
	}

	runtime.DeferredAddComponent<ReplicatedTag>(playerEntity);
	runtime.DeferredUpsertComponent<SpawnTypeComp>(
		playerEntity,
		SpawnTypeComp{
			.characterId = characterDef->id
		});

	_pendingSpawns.push_back(
		PendingCharacterSpawn{
			.sessionId = sessionId,
			.worldId = startupWorldId,
			.entity = playerEntity,
			.characterId = characterDef->id
		});

	context->selectedCharacterId = characterDef->id;
	context->stage = EntryFlowStage::SpawnPending;

	return MakeResult(
		PlayerEntryResultCode::Success,
		sessionId,
		characterDef->id,
		startupWorldId,
		playerEntity);
}

const PendingCharacterSpawn* PlayerEntryService::FindPendingSpawn(SessionId sessionId) const noexcept
{
	const auto it = std::find_if(
		_pendingSpawns.begin(),
		_pendingSpawns.end(),
		[sessionId](const PendingCharacterSpawn& pending)
		{
			return pending.sessionId == sessionId;
		});

	return it != _pendingSpawns.end() ? &(*it) : nullptr;
}

const PendingCharacterSpawn* PlayerEntryService::FindPendingSpawn(
	WorldId worldId,
	Entity entity) const noexcept
{
	const auto it = std::find_if(
		_pendingSpawns.begin(),
		_pendingSpawns.end(),
		[worldId, entity](const PendingCharacterSpawn& pending)
		{
			return pending.worldId == worldId && pending.entity == entity;
		});

	return it != _pendingSpawns.end() ? &(*it) : nullptr;
}

std::span<const PendingCharacterSpawn> PlayerEntryService::GetPendingSpawns() const noexcept
{
	return std::span<const PendingCharacterSpawn>(_pendingSpawns);
}

bool PlayerEntryService::TryConsumeSpawnConfirmed(
	WorldId worldId,
	Entity entity,
	PendingCharacterSpawn& outPendingSpawn)
{
	const auto it = std::find_if(
		_pendingSpawns.begin(),
		_pendingSpawns.end(),
		[worldId, entity](const PendingCharacterSpawn& pending)
		{
			return pending.worldId == worldId && pending.entity == entity;
		});

	if (it == _pendingSpawns.end())
	{
		return false;
	}

	outPendingSpawn = *it;
	_pendingSpawns.erase(it);
	_contexts.erase(outPendingSpawn.sessionId);

	return true;
}

PlayerEntryContext* PlayerEntryService::EnsureContext(SessionId sessionId)
{
	const auto [it, inserted] = _contexts.try_emplace(sessionId);
	(void)inserted;

	PlayerEntryContext& context = it->second;
	context.sessionId = sessionId;
	return &context;
}

bool PlayerEntryService::HasPendingSpawn(SessionId sessionId) const noexcept
{
	return FindPendingSpawn(sessionId) != nullptr;
}
