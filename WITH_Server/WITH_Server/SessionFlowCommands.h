#pragma once

#include <cstdint>

#include "CharacterDef.h"
#include "Entity.h"
#include "Session.h"
#include "SessionFlowTypes.h"
#include "WorldId.h"
#include "WorldIds.h"

struct ISessionCommand
{
	virtual ~ISessionCommand() = default;
	virtual SessionCommandId Id() const noexcept = 0;
};

struct LoginRequested final : ISessionCommand
{
	SessionCommandId Id() const noexcept override
	{
		return SessionCommandId::LoginRequested;
	}
};

struct LoginSucceeded final : ISessionCommand
{
	NetId controlledNetId{ NetId::Invalid() };

	SessionCommandId Id() const noexcept override
	{
		return SessionCommandId::LoginSucceeded;
	}
};

struct LoginFailed final : ISessionCommand
{
	uint32_t reason{ 0 };

	SessionCommandId Id() const noexcept override
	{
		return SessionCommandId::LoginFailed;
	}
};

struct CharacterSelectRequested final : ISessionCommand
{
	CharacterId characterId{ CharacterId::None };

	SessionCommandId Id() const noexcept override
	{
		return SessionCommandId::CharacterSelectRequested;
	}
};

struct CharacterDataLoaded final : ISessionCommand
{
	CharacterId characterId{ CharacterId::None };
	WorldId worldId{ WorldId::Invalid() };

	SessionCommandId Id() const noexcept override
	{
		return SessionCommandId::CharacterDataLoaded;
	}
};

struct CharacterDataLoadFailed final : ISessionCommand
{
	uint32_t reason{ 0 };

	SessionCommandId Id() const noexcept override
	{
		return SessionCommandId::CharacterDataLoadFailed;
	}
};

struct CharacterSpawnConfirmed final : ISessionCommand
{
	WorldId worldId{ WorldId::Invalid() };
	Entity entity{ Entity::Null() };
	NetId netId{ NetId::Invalid() };
	CharacterId characterId{ CharacterId::None };

	SessionCommandId Id() const noexcept override
	{
		return SessionCommandId::CharacterSpawnConfirmed;
	}
};

struct CharacterSpawnFailed final : ISessionCommand
{
	uint32_t reason{ 0 };

	SessionCommandId Id() const noexcept override
	{
		return SessionCommandId::CharacterSpawnFailed;
	}
};

struct ClientWorldReady final : ISessionCommand
{
	TransferId transferId{ 0 };

	SessionCommandId Id() const noexcept override
	{
		return SessionCommandId::ClientWorldReady;
	}
};

struct DisconnectRequested final : ISessionCommand
{
	SessionCloseReason reason{ SessionCloseReason::RemoteClosed };

	SessionCommandId Id() const noexcept override
	{
		return SessionCommandId::DisconnectRequested;
	}
};

struct SessionTimeout final : ISessionCommand
{
	SessionCommandId Id() const noexcept override
	{
		return SessionCommandId::Timeout;
	}
};
