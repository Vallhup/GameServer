#pragma once

#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

#include "CharacterDataService.h"
#include "CharacterSpawnService.h"
#include "ServerPacketStager.h"
#include "NetworkRuntime.h"
#include "PacketHandlerContext.h"
#include "SessionFlowController.h"

class FrameworkRuntime;
class IWorldTransitionRequestSink;

enum class InitialWorldReadyResult : uint8_t
{
	NotInitialEntry,
	Accepted,
	Rejected,
};

class ServerSessionSystem final {
public:
	struct Config
	{
		uint16_t networkThreadCount{ 4 };
		uint16_t listenPort{ 7000 };
		uint32_t maxSessions{ 1024 };
	};

public:
	ServerSessionSystem(
		Config config,
		FrameworkRuntime& framework,
		const WorldId& startupWorldId,
		IWorldTransitionRequestSink& worldTransitionSink);

	bool Initialize();
	void Shutdown() noexcept;
	void ClearSessionState() noexcept;
	void HandleSessionDisconnected(
		SessionId sessionId,
		SessionCloseReason reason) noexcept;

	void BeginSendStage() noexcept;
	void FlushOutbound();

	bool BeginInitialWorldEntry(
		SessionId sessionId,
		WorldId worldId,
		Entity entity,
		NetId playerNetId,
		CharacterId characterId);

	InitialWorldReadyResult MarkInitialWorldReady(
		SessionId sessionId,
		TransferId transferId,
		double nowSec,
		std::span<const SessionId> additionalExcludedSessions = {});

	void AppendPendingInitialEntrySessions(std::vector<SessionId>& outSessionIds) const;

	NetworkRuntime&       Network() noexcept       { return _network; }
	const NetworkRuntime& Network() const noexcept { return _network; }

	SessionFlowController&       Flow() noexcept       { return _sessionFlowController; }
	const SessionFlowController& Flow() const noexcept { return _sessionFlowController; }

	CharacterDataService&  CharacterData() noexcept  { return _characterDataService; }
	CharacterSpawnService& CharacterSpawn() noexcept { return _characterSpawnService; }

private:
	struct PendingInitialEntry
	{
		TransferId  transferId{ 0 };
		SessionId   sessionId{ 0 };
		WorldId     worldId{ WorldId::Invalid() };
		Entity      entity{ Entity::Null() };
		NetId       playerNetId{ NetId::Invalid() };
		CharacterId characterId{ CharacterId::None };
	};

	Config _config;
	FrameworkRuntime& _framework;
	const WorldId&    _startupWorldId;
	IWorldTransitionRequestSink& _worldTransitionSink;

	NetworkRuntime        _network;
	CharacterDataService  _characterDataService;
	CharacterSpawnService _characterSpawnService;
	SessionFlowController _sessionFlowController;
	PacketHandlerContext  _packetHandlerCtx;
	std::unordered_map<SessionId, PendingInitialEntry> _pendingInitialEntries;
	TransferId _nextInitialEntryTransferId{ 1 };
};
