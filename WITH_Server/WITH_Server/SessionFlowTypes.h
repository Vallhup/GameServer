#pragma once

#include <cstdint>

#include "CharacterDef.h"
#include "Entity.h"
#include "Session.h"
#include "WorldId.h"
#include "WorldIds.h"
#include "NetId.h"

enum class SessionStateId : uint8_t
{
	Connected,
	Authenticating,
	AwaitingCharacterSelect,
	CharacterDataLoading,
	SpawningCharacter,
	AwaitingClientWorldReady,
	InGame,
	WorldTransitioning,
	Closing,
};

enum class SessionCommandId : uint8_t
{
	LoginRequested,
	LoginSucceeded,
	LoginFailed,
	CharacterSelectRequested,
	CharacterDataLoaded,
	CharacterDataLoadFailed,
	CharacterSpawnConfirmed,
	CharacterSpawnFailed,
	ClientWorldReady,
	DisconnectRequested,
	Timeout,
};

struct SessionFlow
{
	SessionId      sessionId{ 0 };
	SessionStateId stateId{ SessionStateId::Connected };
	NetId          playerNetId{ NetId::Invalid() };
	CharacterId    selectedCharacterId{ CharacterId::None };
	WorldId        playerWorldId{ WorldId::Invalid() };
	Entity         playerEntity{ Entity::Null() };
	TransferId     pendingTransferId{ 0 };
	WorldId        pendingTargetWorldId{ WorldId::Invalid() };

	// SessionBindingRegistry에서 이관된 바인딩 필드 (SSOT)
	NetId  controlledNetId{ NetId::Invalid() };
	WorldId currentWorldId{ WorldId::Invalid() };

	bool HasPlayerNetId() const noexcept
	{
		return playerNetId.IsValid();
	}

	bool HasPendingTransfer() const noexcept
	{
		return pendingTransferId != 0;
	}

	bool HasBinding() const noexcept
	{
		return controlledNetId.IsValid();
	}
};
