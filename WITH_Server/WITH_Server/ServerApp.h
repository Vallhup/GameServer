#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "FrameworkRuntime.h"
#include "GameDataCatalog.h"
#include "GameplayContentCatalog.h"
#include "AnimationJsonLoader.h"
#include "AnimationRegistry.h"
#include "DemoPartyFormationPolicy.h"
#include "IWorldTransitionRequestSink.h"
#include "ODBCDatabaseBackend.h"
#include "PartyCommandPump.h"
#include "PartyCommandQueue.h"
#include "PartyPersistGateway.h"
#include "PartyPersistTypes.h"
#include "PartyService.h"
#include "ServerSessionSystem.h"
#include "ServerWorldBootstrap.h"
#include "ServerWorldTransferBinding.h"

class ServerApp final
	: public IWorldTransitionRequestSink
	, private IPartySessionQuery
	, private IDemoPartyTransitionQuery {
public:
	struct Config
	{
		uint16_t networkThreadCount{ 4 };
		uint16_t listenPort{ 7000 };

		uint32_t logicTickHz{ 30 };
		uint32_t executorWorkerCount{ 4 };

		uint32_t maxSessions{ 1024 };
		uint32_t maxWorlds{ 128 };
		bool enableCheats{ false };

		AccountCombatStatOverride accountCombatStatOverride;
		ODBCDatabaseBackend::Config database;
	};

public:
	explicit ServerApp(Config config = {});
	~ServerApp() = default;

	ServerApp(const ServerApp&) = delete;
	ServerApp& operator=(const ServerApp&) = delete;

	bool Initialize();
	void Run();
	void Stop() noexcept;
	void Shutdown() noexcept;

	bool IsInitialized() const noexcept { return _initialized.load(); }
	bool IsRunning() const noexcept { return _running.load(); }

	const Config& GetConfig() const noexcept { return _config; }
	uint64_t TickCount() const noexcept { return _tickCount; }
	const AnimationRegistry& GetAnimationRegistry() const noexcept { return _animationRegistry; }

	TransferId RequestSessionWorldTransfer(
		SessionId sessionId,
		WorldDefId targetWorldDefId,
		uint64_t instanceKey,
		PartyId partyId,
		bool allowFallback);

	TransferId RequestDebugWorldTransfer(
		SessionId sessionId,
		WorldDefId targetWorldDefId,
		uint64_t instanceKey = 0,
		bool allowFallback = true);

	TransferId RequestDebugTransferToVillage(SessionId sessionId);
	bool RequestPlayerRespawn(SessionId sessionId);

	TransferId RequestDemoWorldTransition(
		SessionId sessionId,
		uint32_t requestId) override;

	TransferId RequestCheatFinalWorldTransition(
		SessionId sessionId,
		uint32_t requestId) override;

	bool SubmitFinalClearPvpChoice(
		SessionId sessionId,
		uint64_t voteId,
		bool choosePvp) override;

	bool RequestBeaconCinematicStart(
		SessionId sessionId,
		uint32_t clientRequestId) override;

	bool SubmitFinalEndingCinematicDone(
		SessionId sessionId,
		uint32_t context) override;

	bool MarkClientWorldTransitionReady(
		SessionId sessionId,
		TransferId transferId) override;

	void OnSessionDisconnected(SessionId sessionId) noexcept override;

private:
	struct PendingClientTransition
	{
		TransferId transferId{ 0 };
		SessionId sessionId{ 0 };
		WorldId targetWorldId{ WorldId::Invalid() };
		NetId playerNetId{ NetId::Invalid() };
		uint32_t mapResourceId{ 0 };
	};

	struct FinalClearChoiceVote
	{
		uint64_t voteId{ 0 };
		PartyId partyId{ 0 };
		WorldId sourceWorldId{ WorldId::Invalid() };
		std::unordered_set<SessionId> eligibleSessions;
		std::unordered_map<SessionId, bool> choices;
		double deadlineSec{ 0.0 };
	};

	struct PendingFinalClearChoiceStart
	{
		WorldId sourceWorldId{ WorldId::Invalid() };
		double startAtSec{ 0.0 };
	};

	struct ActivePvpRound
	{
		PartyId partyId{ 0 };
		WorldId worldId{ WorldId::Invalid() };
		double startedAtSec{ 0.0 };
		bool ending{ false };
		double endingSyncAtSec{ 0.0 };
		uint64_t winnerNetId{ 0 };
	};

	struct ActiveBeaconCinematic
	{
		PartyId partyId{ 0 };
		WorldId sourceWorldId{ WorldId::Invalid() };
		uint64_t cinematicInstanceId{ 0 };
		uint32_t cinematicType{ 0 };
		double expiresAtSec{ 0.0 };
	};

	// 엔딩/페이드 연출 후 Plaza로 전이하기 위한 대기 상태. 연출 신호를 보낸 뒤
	// 파티원 전원의 연출 완료(CS_FINAL_ENDING_CINEMATIC_DONE) 또는 타임아웃 시
	// 강제 그룹 전송을 실행한다.
	struct PendingEndingTransfer
	{
		PartyId partyId{ 0 };
		WorldId sourceWorldId{ WorldId::Invalid() };
		WorldDefId targetWorldDefId{ WorldDefId::Plaza };
		std::unordered_set<SessionId> awaitingAck;
		double deadlineSec{ 0.0 };
	};

private:
	bool InitializeFrameworkRuntime();
	bool InitializeDatabaseRuntime();
	bool InitializeSessionSystem();
	bool InitializeGameplayContent();
	bool InitializeStartupWorld();
	void ShutdownDatabaseRuntime() noexcept;

	void RunLogicLoop();
	void TickOnce(double dtSec);

	void RunWorldFrames(double dtSec);
	void FlushOutbound();
	bool StageWorldTransferSourceRemovals(
		const WorldTransferEventBatch& transferEvents);
	bool StageWorldTransitionBeginPackets(
		const WorldTransferEventBatch& transferEvents);
	void ApplyPartyWorldTransferEvents(
		const WorldTransferEventBatch& transferEvents);
	bool ApplyPartyDeathCountEvents(
		const FrameworkRuntime::FrameResult& frameResult);
	bool IsPartyWipedInWorld(
		PartyId partyId,
		WorldId sourceWorldId);
	bool ApplyFinalBossDefeatedEvents(
		const FrameworkRuntime::FrameResult& frameResult);
	void TickPendingFinalClearChoiceStarts();
	void TickFinalClearChoiceVotes();
	bool StartFinalClearChoiceVote(WorldId sourceWorldId);
	bool ResolveFinalClearChoiceVote(
		uint64_t voteId,
		WorldDefId targetWorldDefId,
		uint32_t reason,
		SessionId pvpChooserSessionId = 0);
	// 캐스팅된 표 기준 판정: 한 명이라도 PvP면 PvP, 아니면 Plaza(plazaReason).
	bool ResolveFinalClearVoteByCastChoices(uint64_t voteId, uint32_t plazaReason);
	bool RequestPartyWorldTransfer(PartyId partyId, WorldDefId targetWorldDefId);
	// 리더 비의존 강제 그룹 전송(엔딩/PvP 종료 복귀용).
	bool ForcePartyGroupTransfer(
		PartyId partyId,
		WorldId sourceWorldId,
		WorldDefId targetWorldDefId);
	// 엔딩 연출 핸드셰이크: 신호 송신 후 전원 완료/타임아웃 시 전이.
	void BeginEndingThenTransfer(
		PartyId partyId,
		WorldId sourceWorldId,
		WorldDefId targetWorldDefId,
		std::span<const SessionId> members);
	// 투표 없는 엔딩(솔로/자격<2): ENDING 결과 송신 후 연출→Plaza 핸드셰이크 시작.
	void StartFinalEndingForParty(PartyId partyId, WorldId sourceWorldId);
	void TickPendingEndingTransfers();
	bool ProcessPvpRoundEndConditions();
	bool StagePartyDeathCountSync(
		PartyId partyId,
		const PartyDeathCountState& deathCount);
	bool ApplyPlayerDeathCountDecision(
		const FrameworkRuntime::FrameResult::PlayerDeathCountEvent& deathEvent,
		bool canRespawn,
		uint64_t deathCountRevision);

	bool IsPartyEligible(SessionId sessionId) const override;
	uint64_t FindAccountId(SessionId sessionId) const override;
	CharacterId FindSelectedCharacterId(SessionId sessionId) const override;
	NetId FindControlledNetId(SessionId sessionId) const override;
	WorldId FindCurrentWorldId(SessionId sessionId) const override;
	void CollectSessionsInWorld(
		WorldId worldId,
		std::vector<SessionId>& outSessionIds) const override;
	bool CanBeginWorldTransfer(SessionId sessionId) const override;
	bool IsClientTransitionPending(SessionId sessionId) const override;

private:
	Config _config;

	std::atomic<bool> _initialized{ false };
	std::atomic<bool> _running{ false };
	std::atomic<bool> _stopRequested{ false };

	ServerWorldBootstrapFactory _bootstrapFactory;
	ServerWorldBootstrapDefinitionProvider _bootstrapDefinitions;
	FrameworkRuntime _framework;
	AnimationJsonLoader _animationLoader;
	AnimationRegistry _animationRegistry;
	GameDataCatalog _gameDataCatalog;
	GameplayContentCatalogSnapshot _gameplayContentCatalog;
	WorldId _startupWorldId{};
	std::unique_ptr<ODBCDatabaseBackend> _databaseBackend;
	ServerSessionSystem _sessionSystem;
	ServerWorldTransferBinding _transferBinding;
	PartyService _partyService;
	PartyCommandQueue _partyCommandQueue;
	PartyCommandPump _partyCommandPump;
	PartyDbResultQueue _partyDbResultQueue;
	std::unique_ptr<PartyPersistGateway> _partyPersistGateway;
	DemoPartyFormationPolicy _demoPartyPolicy;
	std::unordered_map<TransferId, std::unordered_map<SessionId, uint32_t>>
		_worldTransitionRequestIds;
	std::unordered_map<SessionId, PendingClientTransition> _pendingClientTransitions;
	std::unordered_map<TransferId, std::unordered_set<SessionId>>
		_activeClientTransitionSessions;
	std::unordered_map<uint64_t, PendingFinalClearChoiceStart>
		_pendingFinalClearChoiceStartByWorld;
	std::unordered_map<uint64_t, FinalClearChoiceVote> _finalClearChoiceVotes;
	std::unordered_map<uint64_t, uint64_t> _finalClearChoiceVoteByWorld;
	std::unordered_map<uint64_t, ActivePvpRound> _activePvpRounds;
	std::unordered_map<uint64_t, ActiveBeaconCinematic>
		_activeBeaconCinematicsByWorld;
	// 키: partyId. 엔딩 연출 완료 대기 중인 파티들.
	std::unordered_map<uint64_t, PendingEndingTransfer> _pendingEndingTransfers;
	uint64_t _nextFinalClearChoiceVoteId{ 1 };
	uint64_t _nextBeaconCinematicInstanceId{ 1 };

	uint64_t _tickCount{ 0 };
	uint64_t _frameIndex{ 0 };
	double _nowSec{ 0.0 };
	std::chrono::steady_clock::time_point _lastTickTime{};
};
