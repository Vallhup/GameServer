#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "CharacterDataService.h"
#include "Entity.h"
#include "Session.h"
#include "WorldId.h"

class FrameworkRuntime;

struct PendingSessionCharacterSpawn
{
	SessionId sessionId{ 0 };
	WorldId worldId{ WorldId::Invalid() };
	Entity entity{ Entity::Null() };
	CharacterId characterId{ CharacterId::None };
	NetId netId{ NetId::Invalid() };
};

enum class CharacterSpawnResultCode : uint8_t
{
	Success,
	InvalidSession,
	InvalidCharacterData,
	InvalidReservedNetId,
	WorldNotFound,
	EntityReserveFailed,
	NetBindFailed,
	DuplicatePendingSpawn,
};

struct CharacterSpawnResult
{
	CharacterSpawnResultCode code{ CharacterSpawnResultCode::InvalidSession };
	SessionId sessionId{ 0 };
	WorldId worldId{ WorldId::Invalid() };
	Entity entity{ Entity::Null() };
	CharacterId characterId{ CharacterId::None };
	NetId netId{ NetId::Invalid() };

	bool Succeeded() const noexcept
	{
		return code == CharacterSpawnResultCode::Success;
	}
};

class CharacterSpawnService final {
public:
	struct Dependencies
	{
		FrameworkRuntime* framework{ nullptr };
	};

public:
	explicit CharacterSpawnService(Dependencies deps = {});

	void SetDependencies(Dependencies deps) noexcept;
	void Clear() noexcept;

	CharacterSpawnResult RequestCharacterSpawn(
		const CharacterDataResult& data,
		NetId reservedPlayerNetId);

	bool CancelPendingSpawn(SessionId sessionId) noexcept;

	const PendingSessionCharacterSpawn* FindPendingSpawn(SessionId sessionId) const noexcept;
	const PendingSessionCharacterSpawn* FindPendingSpawn(
		WorldId worldId,
		Entity entity) const noexcept;

	std::span<const PendingSessionCharacterSpawn> GetPendingSpawns() const noexcept;

	bool TryConsumeSpawnConfirmed(
		WorldId worldId,
		Entity entity,
		PendingSessionCharacterSpawn& outPendingSpawn);

private:
	bool HasPendingSpawn(SessionId sessionId) const noexcept;

	Dependencies _deps;
	std::vector<PendingSessionCharacterSpawn> _pendingSpawns;
};
