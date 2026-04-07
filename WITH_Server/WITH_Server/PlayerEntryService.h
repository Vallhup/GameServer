#pragma once

#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

#include "CharacterDef.h"
#include "FrameworkRuntime.h"
#include "Session.h"

// TODO : 서버 -> 클라 -> title -> esc 끄기 -> 다시 클라 키면 터짐

enum class EntryFlowStage : uint8_t
{
	None,
	AwaitingCharacterSelect,
	SpawnPending,
};

struct PlayerEntryContext
{
	SessionId sessionId{ 0 };
	EntryFlowStage stage{ EntryFlowStage::None };
	std::optional<CharacterId> selectedCharacterId;

	bool HasSelectedCharacter() const noexcept
	{
		return selectedCharacterId.has_value();
	}
};

struct PendingCharacterSpawn
{
	SessionId sessionId{ 0 };
	WorldId worldId{ WorldId::Invalid() };
	Entity entity{ Entity::Null() };
	CharacterId characterId{ CharacterId::None };
};

enum class PlayerEntryResultCode : uint8_t
{
	Success,
	InvalidSession,
	EntryNotStarted,
	InvalidFlowStage,
	CharacterIdNotPlayable,
	CharacterDefNotFound,
	StartupWorldUnavailable,
	WorldNotFound,
	EntityReserveFailed,
	DuplicatePendingSpawn,
};

struct PlayerEntryResult
{
	PlayerEntryResultCode code{ PlayerEntryResultCode::InvalidSession };
	SessionId sessionId{ 0 };
	WorldId worldId{ WorldId::Invalid() };
	Entity entity{ Entity::Null() };
	CharacterId characterId{ CharacterId::None };

	bool Succeeded() const noexcept
	{
		return code == PlayerEntryResultCode::Success;
	}
};

class PlayerEntryService final {
public:
	struct Dependencies
	{
		FrameworkRuntime* framework{ nullptr };
		const WorldId* startupWorldId{ nullptr };
	};

public:
	explicit PlayerEntryService(Dependencies deps = {});

	void SetDependencies(Dependencies deps) noexcept;

	void Clear() noexcept;

	// Called after the caller has already completed network-side login/auth.
	bool BeginAuthenticatedEntry(SessionId sessionId);

	// Cancels any in-progress entry flow for the given session.
	bool CancelEntry(SessionId sessionId) noexcept;

	const PlayerEntryContext* FindContext(SessionId sessionId) const noexcept;
	PlayerEntryContext* FindContext(SessionId sessionId) noexcept;

	EntryFlowStage FindStage(SessionId sessionId) const noexcept;
	bool IsAwaitingCharacterSelect(SessionId sessionId) const noexcept;
	bool IsSpawnPending(SessionId sessionId) const noexcept;

	// Temporary internal entry point. A future packet handler should call this API.
	PlayerEntryResult RequestCharacterSelect(
		SessionId sessionId,
		CharacterId characterId);

	const PendingCharacterSpawn* FindPendingSpawn(SessionId sessionId) const noexcept;
	const PendingCharacterSpawn* FindPendingSpawn(
		WorldId worldId,
		Entity entity) const noexcept;

	std::span<const PendingCharacterSpawn> GetPendingSpawns() const noexcept;

	// Called when the world frame reports that a reserved player entity was spawned.
	bool TryConsumeSpawnConfirmed(
		WorldId worldId,
		Entity entity,
		PendingCharacterSpawn& outPendingSpawn);

private:
	PlayerEntryContext* EnsureContext(SessionId sessionId);
	bool HasPendingSpawn(SessionId sessionId) const noexcept;

private:
	Dependencies _deps;
	std::unordered_map<SessionId, PlayerEntryContext> _contexts;
	std::vector<PendingCharacterSpawn> _pendingSpawns;
};
