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
	// controlledNetId: 로그인 성공 시 할당, 월드 바인딩 이후 역인덱스에도 등록됨
	// HasBinding()은 월드 바인딩 완료(currentWorldId 유효) 여부를 의미한다
	uint64_t       accountId{ 0 };
	NetId          controlledNetId{ NetId::Invalid() };
	CharacterId    selectedCharacterId{ CharacterId::None };
	WorldId        playerWorldId{ WorldId::Invalid() };
	Entity         playerEntity{ Entity::Null() };
	TransferId     pendingTransferId{ 0 };
	WorldId        pendingTargetWorldId{ WorldId::Invalid() };
	WorldId        currentWorldId{ WorldId::Invalid() };

	bool HasPendingTransfer() const noexcept
	{
		return pendingTransferId != 0;
	}

	bool HasBinding() const noexcept
	{
		return controlledNetId.IsValid() && currentWorldId.IsValid();
	}
};
