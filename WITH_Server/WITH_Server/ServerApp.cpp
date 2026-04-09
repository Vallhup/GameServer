#include "pch.h"
#include "ServerApp.h"

#include <algorithm>
#include <cstdint>
#include <thread>

#include "PacketFactory.h"
#include "Protocol.pb.h"
#include "ECS/GameplayRuntimeComponents.h"
#include "RepComponent.h"
#include "WorldInstance.h"
#include <filesystem>

#include "TransformHelper.h"

namespace
{
	std::filesystem::path GetDefaultAnimationOutputRoot()
	{
		return std::filesystem::current_path() /
			"..\\Animation";
	}

	std::vector<std::filesystem::path> GetBootAnimationCandidates(
		const std::filesystem::path& root)
	{
		return
		{
			root / "Imp" / "imp_animation_death_1.json",
			root / "Imp" / "imp_animation_death_2.json",
			root / "Imp" / "imp_animation_idle_1.json",
			root / "Imp" / "imp_animation_idle_2.json",
			root / "Imp" / "imp_animation_idle_3.json",
			root / "Imp" / "imp_animation_idle_4.json",
			root / "Imp" / "imp_animation_idle_5.json",
			root / "Imp" / "imp_animation_idle_6.json",
			root / "Imp" / "imp_animation_idle_battlecry.json",
			root / "Imp" / "imp_animation_idle_roaring.json",
			root / "Imp" / "imp_animation_jump_1.json",
			root / "Imp" / "imp_animation_melee_1.json",
			root / "Imp" / "imp_animation_melee_2.json",
			root / "Imp" / "imp_animation_melee_3.json",
			root / "Imp" / "imp_animation_melee_4.json",
			root / "Imp" / "imp_animation_melee_5.json",
			root / "Imp" / "imp_animation_react_front.json",
			root / "Imp" / "imp_animation_react_left.json",
			root / "Imp" / "imp_animation_react_right.json",
			root / "Imp" / "imp_animation_stun.json",
			root / "Imp" / "imp_animation_walk_back.json",
			root / "Imp" / "imp_animation_walk_forward.json",
			root / "Imp" / "imp_animation_walk_left.json",
			root / "Imp" / "imp_animation_walk_right.json",


			root / "Knight" / "knight_animation_death.json",
			root / "Knight" / "knight_animation_dodge.json",
			root / "Knight" / "knight_animation_drinking.json",
			root / "Knight" / "knight_animation_guard.json",
			root / "Knight" / "knight_animation_heavyattack.json",
			root / "Knight" / "knight_animation_hit.json",
			root / "Knight" / "knight_animation_idle.json",
			root / "Knight" / "knight_animation_lightattack1.json",
			root / "Knight" / "knight_animation_lightattack2.json",
			root / "Knight" / "knight_animation_lightattack3.json",
			root / "Knight" / "knight_animation_parry.json",
			root / "Knight" / "knight_animation_run.json",
			root / "Knight" / "knight_animation_specialattack.json",
			root / "Knight" / "knight_animation_stun.json",
			root / "Knight" / "knight_animation_walk.json",


			root / "Final_Boss" / "final_boss_animation_dashslash.json",
			root / "Final_Boss" / "final_boss_animation_death.json",
			root / "Final_Boss" / "final_boss_animation_hit.json",
			root / "Final_Boss" / "final_boss_animation_idle.json",
			root / "Final_Boss" / "final_boss_animation_jumpslash.json",
			root / "Final_Boss" / "final_boss_animation_multislash.json",
			root / "Final_Boss" / "final_boss_animation_slash.json",
			root / "Final_Boss" / "final_boss_animation_stun.json",
			root / "Final_Boss" / "final_boss_animation_thrust.json",
			root / "Final_Boss" / "final_boss_animation_walk.json",
		};
	}

	bool StageLoginResponse(
		NetworkRuntime& network,
		SessionId sessionId,
		NetId playerNetId)
	{
		Protocol::SC_LOGIN_PACKET loginAck;
		loginAck.set_netid(playerNetId.GetRaw());

		SendBuffer* const buffer =
			PacketFactory::Serialize(PacketType::SC_LOGIN, loginAck);
		if (buffer == nullptr)
		{
			return false;
		}

		const bool staged = 
			network.StageUnicast(sessionId, std::span<const uint8_t>(buffer->data, buffer->size));

		SendBufferPool::Get().Release(buffer);
		return staged;
	}

	bool StageSpawnAddPacketToSession(
		NetworkRuntime& network,
		SessionId sessionId,
		NetId netId,
		CharacterId characterId,
		const WorldTransformComp* transform = nullptr)
	{
		Protocol::SC_ADD_PACKET add;
		add.set_netid(netId.GetRaw());
		add.set_typeid_(static_cast<int>(characterId));
		add.set_x(transform != nullptr ? transform->position.x : 0.0f);
		add.set_y(transform != nullptr ? transform->position.y : 0.0f);
		add.set_z(transform != nullptr ? transform->position.z : 0.0f);
		add.set_yaw(transform != nullptr ? 
			TransformHelper::QuaternionToYaw(transform->rotation) : 0.0f);
		SendBuffer* const buffer =
			PacketFactory::Serialize(PacketType::SC_ADD, add);
		if (buffer == nullptr)
		{
			return false;
		}
		const bool staged =
			network.StageUnicast(sessionId, std::span<const uint8_t>(buffer->data, buffer->size));
		SendBufferPool::Get().Release(buffer);
		return staged;
	}

	bool StageSpawnAddPacketToSessions(
		NetworkRuntime& network,
		std::span<const SessionId> sessionIds,
		NetId netId,
		CharacterId characterId,
		const WorldTransformComp* transform = nullptr)
	{
		if (sessionIds.empty())
		{
			return true;
		}
		Protocol::SC_ADD_PACKET add;
		add.set_netid(netId.GetRaw());
		add.set_typeid_(static_cast<int>(characterId));
		add.set_x(transform != nullptr ? transform->position.x : 0.0f);
		add.set_y(transform != nullptr ? transform->position.y : 0.0f);
		add.set_z(transform != nullptr ? transform->position.z : 0.0f);
		add.set_yaw(transform != nullptr ?
			TransformHelper::QuaternionToYaw(transform->rotation) : 0.0f);
		SendBuffer* const buffer =
			PacketFactory::Serialize(PacketType::SC_ADD, add);
		if (buffer == nullptr)
		{
			return false;
		}
		const bool staged = network.StageMulticast(
			sessionIds,
			std::span<const uint8_t>(buffer->data, buffer->size));
		SendBufferPool::Get().Release(buffer);
		return staged;
	}

	bool TryGetReplicatedSpawnState(
		FrameworkRuntime& framework,
		WorldId worldId,
		Entity entity,
		CharacterId& outCharacterId,
		const WorldTransformComp*& outTransform)
	{
		WorldInstance* const world = framework.FindWorld(worldId);
		if (world == nullptr)
		{
			return false;
		}
		ECSView view = world->GetRuntime().MakeView();
		if (!view.HasComponent<ReplicatedTag>(entity))
		{
			return false;
		}
		const SpawnTypeComp* const spawnType = view.GetComponent<SpawnTypeComp>(entity);
		if (spawnType == nullptr)
		{
			return false;
		}
		outCharacterId = spawnType->characterId;
		outTransform = view.GetComponent<WorldTransformComp>(entity);
		return true;
	}

	void StageExistingWorldEntitiesForSession(
		FrameworkRuntime& framework,
		NetworkRuntime& network,
		WorldId worldId,
		SessionId sessionId,
		NetId excludedNetId = NetId::Invalid())
	{
		WorldInstance* const world = framework.FindWorld(worldId);
		if (world == nullptr)
		{
			return;
		}
		ECSView view = world->GetRuntime().MakeView();
		for (auto [entity, spawnType] : view.View<SpawnTypeComp>())
		{
			if (!view.HasComponent<ReplicatedTag>(entity))
			{
				continue;
			}
			
			const NetId entityNetId = framework.FindNetId(worldId, entity);
			if (!entityNetId.IsValid() || entityNetId == excludedNetId)
			{
				continue;
			}
			const WorldTransformComp* const transform =
				view.GetComponent<WorldTransformComp>(entity);
			(void)StageSpawnAddPacketToSession(
				network,
				sessionId,
				entityNetId,
				spawnType.characterId,
				transform);
		}
	}
	bool AssignPlayerControlNetId(
		FrameworkRuntime& framework,
		WorldId worldId,
		Entity entity,
		NetId netId)
	{
		WorldInstance* const world = framework.FindWorld(worldId);
		if (world == nullptr)
		{
			return false;
		}

		ECSView view = world->GetRuntime().MakeView();
		auto* identity = const_cast<PlayerControlIdentityComp*>(
			view.GetComponent<PlayerControlIdentityComp>(entity));
		if (identity == nullptr)
		{
			return false;
		}

		identity->netId = netId;
		return true;
	}

	template<typename TPacket>
	bool StageReplicationPacket(
		NetworkRuntime& network,
		PacketType packetType,
		std::span<const SessionId> sessionIds,
		const TPacket& packet)
	{
		if (sessionIds.empty())
		{
			return true;
		}

		SendBuffer* const buffer =
			PacketFactory::Serialize(packetType, packet);
		if (buffer == nullptr)
		{
			return false;
		}

		const bool staged = network.StageMulticast(
			sessionIds,
			std::span<const uint8_t>(buffer->data, buffer->size));
		SendBufferPool::Get().Release(buffer);
		return staged;
	}
}

ServerApp::ServerApp(Config config)
	: _config(config)
	, _framework(FrameworkRuntime::Config{
		.executorWorkerCount = _config.executorWorkerCount
	})
	, _network(NetworkRuntime::Config{
		.workerThreadCount = _config.networkThreadCount,
		.listenPort = _config.listenPort,
		.maxSessions = _config.maxSessions
	})
	, _playerEntryService(PlayerEntryService::Dependencies{
		&_framework,
		&_startupWorldId
	})
	, _inboundProcessor(InboundMessageProcessor::Dependencies{
		&_framework,
		&_network,
		&_playerEntryService,
		&_sessionBindings,
	})
{
	if (_config.logicTickHz == 0)
	{
		_config.logicTickHz = 30;
	}
}

bool ServerApp::Initialize()
{
	bool expected = false;
	if (!_initialized.compare_exchange_strong(expected, true))
	{
		return true;
	}

	_stopRequested.store(false);
	_tickCount = 0;
	_frameIndex = 1;
	_nowSec = 0.0;
	_startupWorldId = WorldId::Invalid();
	_sessionBindings.Clear();
	_playerEntryService.Clear();
	_animationRegistry.Clear();
	_lastTickTime = {};

	if (!InitializeGameplayContent() ||
		!InitializeFrameworkRuntime() ||
		!InitializeNetworkRuntime())
	{
		_network.Shutdown();
		_framework.Shutdown();
		_initialized.store(false);
		return false;
	}

	return true;
}

void ServerApp::Run()
{
	if (_running.load())
	{
		return;
	}

	if (!IsInitialized() && !Initialize())
	{
		return;
	}

	_stopRequested.store(false);
	_running.store(true);

	std::cout << "[ServerApp] ServerApp Initialize Success.\n";

	RunLogicLoop();
	_running.store(false);
}

void ServerApp::Stop() noexcept
{
	_stopRequested.store(true);
}

void ServerApp::Shutdown() noexcept
{
	Stop();
	_running.store(false);
	_network.Shutdown();
	_framework.Shutdown();
	_initialized.store(false);
	_tickCount = 0;
	_frameIndex = 1;
	_nowSec = 0.0;
	_startupWorldId = WorldId::Invalid();
	_sessionBindings.Clear();
	_playerEntryService.Clear();
	_animationRegistry.Clear();
	_lastTickTime = {};
}

bool ServerApp::InitializeFrameworkRuntime()
{
	_bootstrapFactory.SetAnimationRegistry(&_animationRegistry);
	_bootstrapFactory.SetFramework(&_framework);
	_bootstrapFactory.SetBootstrapWorldId(&_startupWorldId);

	FrameworkRuntime::BootstrapParams bootstrapParams{};
	bootstrapParams.worldFactory = &_bootstrapFactory;
	bootstrapParams.definitionProvider = &_bootstrapDefinitions;

	if (!_framework.Initialize(bootstrapParams))
	{
		std::cout << "[ServerApp] Framework bootstrap failed.\n";
		return false;
	}

	return InitializeStartupWorld();
}

bool ServerApp::InitializeNetworkRuntime()
{
	if (!_network.Initialize())
	{
		std::cout << "[ServerApp] Network runtime initialize failed.\n";
		return false;
	}

	_network.Start();
	return true;
}

bool ServerApp::InitializeGameplayContent()
{
	_animationRegistry.Clear();

	const std::filesystem::path animationRoot = GetDefaultAnimationOutputRoot();
	const std::vector<std::filesystem::path> candidates =
		GetBootAnimationCandidates(animationRoot);

	std::vector<AnimationClipDef> animationDefs;
	for (const auto& path : candidates)
	{
		if (!std::filesystem::exists(path))
		{
			continue;
		}

		AnimationClipDef def;
		const auto loadResult = _animationLoader.LoadFile(path, def);
		if (!loadResult.succeeded)
		{
			std::cout << "[ServerApp] Animation load failed."
				<< " path=" << path.string()
				<< " error=" << loadResult.error << "\n";
			return false;
		}

		animationDefs.push_back(std::move(def));
	}

	if (animationDefs.empty())
	{
		std::cout << "[ServerApp] No animation json files were loaded."
			<< " root=" << animationRoot.string() << "\n";
		return false;
	}

	const AnimationRegistry::BuildResult buildResult =
		_animationRegistry.Rebuild(std::move(animationDefs));
	if (!buildResult.succeeded)
	{
		std::cout << "[ServerApp] Animation registry build failed."
			<< " error=" << buildResult.error << "\n";
		return false;
	}

	std::cout << "[ServerApp] Animation content loaded."
		<< " clips=" << buildResult.loadedCount << "\n";
	return true;
}

bool ServerApp::InitializeStartupWorld()
{
	// Square is currently modeled as a pre-created persistent hub world.
	constexpr WorldDefId startupWorldDefId = WorldDefId::Square;
	constexpr uint64_t startupInstanceKey = 0;

	_startupWorldId = _framework.RegisterPreCreatedWorld(
		startupWorldDefId,
		startupInstanceKey);

	if (!_startupWorldId.IsValid())
	{
		std::cout << "[ServerApp] Startup world creation failed."
			<< " defId=" << static_cast<int>(startupWorldDefId)
			<< " instanceKey=" << startupInstanceKey << "\n";
		return false;
	}

	if (!_framework.InitializeWorld(_startupWorldId))
	{
		std::cout << "[ServerApp] Startup world initialization failed."
			<< " worldId=" << _startupWorldId.GetRaw() << "\n";
		return false;
	}

	return true;
}

void ServerApp::RunLogicLoop()
{
	using clock = std::chrono::steady_clock;

	const double fixedDt = 1.0 / static_cast<double>(_config.logicTickHz);
	auto previous = clock::now();
	double accumulator = 0.0;

	while (!_stopRequested.load())
	{
		const auto now = clock::now();
		accumulator += std::chrono::duration<double>(now - previous).count();
		previous = now;

		while (accumulator >= fixedDt && !_stopRequested.load())
		{
			TickOnce(fixedDt);
			accumulator -= fixedDt;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void ServerApp::TickOnce(double dtSec)
{
	_lastTickTime = std::chrono::steady_clock::now();

	DrainInboundCommands();
	_network.BeginSendStage();
	ProcessInboundMessages();
	RunWorldFrames(dtSec);
	BuildReplication();
	FlushOutbound();

	++_tickCount;
}

void ServerApp::DrainInboundCommands()
{
	_network.DrainInboundMessages(_inboundMessages);
}

void ServerApp::ProcessInboundMessages()
{
	_inboundProcessor.Process(_inboundMessages, _remainingInboundMessages);
	_inboundMessages.swap(_remainingInboundMessages);
}

void ServerApp::RunWorldFrames(double dtSec)
{
	_nowSec += dtSec;

	if (!_framework.TickServices(_nowSec, dtSec))
	{
		std::cout << "[ServerApp] Framework service tick failed.\n";
		Stop();
		return;
	}

	FrameworkRuntime::FrameResult frameResult{};
	FrameworkRuntime::FrameParams frameParams{};
	frameParams.frameIndex = _frameIndex;
	frameParams.nowSec = _nowSec;
	frameParams.dtSec = dtSec;

	const bool frameOk =
		_framework.RunFrame(frameParams, frameResult);

	if (!frameOk)
	{
		std::cout << "[ServerApp] Frame execution failed."
			<< " frameIndex=" << _frameIndex
			<< " reason=" << static_cast<int>(frameResult.failureReason)
			<< " selectedWorlds=" << frameResult.selectedWorldCount
			<< "\n";
		Stop();
		return;
	}

	FinalizeFrameEvents(frameResult);
	++_frameIndex;
}

void ServerApp::FinalizeFrameEvents(const FrameworkRuntime::FrameResult& frameResult)
{
	std::vector<SessionId> worldSessionIds;
	std::vector<SessionId> otherSessionIds;
	for (const auto& spawnEvent : frameResult.events.spawns)
	{
		if (!spawnEvent.netId.IsValid())
		{
			continue;
		}
		CharacterId characterId = CharacterId::None;
		const WorldTransformComp* transform = nullptr;
		if (!TryGetReplicatedSpawnState(
			_framework,
			spawnEvent.worldId,
			spawnEvent.entity,
			characterId,
			transform))
		{
			continue;
		}
		const PendingCharacterSpawn* pendingCharacterSpawn =
			_playerEntryService.FindPendingSpawn(spawnEvent.worldId, spawnEvent.entity);
		if (pendingCharacterSpawn == nullptr)
		{
			_sessionBindings.CollectSessionsInWorld(spawnEvent.worldId, worldSessionIds);
			(void)StageSpawnAddPacketToSessions(
				_network,
				std::span<const SessionId>(worldSessionIds),
				spawnEvent.netId,
				characterId,
				transform);
			continue;
		}
		PendingCharacterSpawn pendingSpawn{};
		if (!_playerEntryService.TryConsumeSpawnConfirmed(
			spawnEvent.worldId,
			spawnEvent.entity,
			pendingSpawn))
		{
			continue;
		}
		const SessionId sessionId = pendingSpawn.sessionId;
		(void)AssignPlayerControlNetId(
			_framework,
			spawnEvent.worldId,
			spawnEvent.entity,
			spawnEvent.netId);
		(void)_sessionBindings.Bind(sessionId, spawnEvent.netId, spawnEvent.worldId);
		(void)_network.RequestEnterInGame(sessionId, spawnEvent.netId);
		(void)StageLoginResponse(_network, sessionId, spawnEvent.netId);
		(void)StageSpawnAddPacketToSession(
			_network,
			sessionId,
			spawnEvent.netId,
			characterId,
			transform);
		StageExistingWorldEntitiesForSession(
			_framework,
			_network,
			spawnEvent.worldId,
			sessionId,
			spawnEvent.netId);
		_sessionBindings.CollectSessionsInWorld(spawnEvent.worldId, worldSessionIds);
		otherSessionIds.clear();
		for (SessionId worldSessionId : worldSessionIds)
		{
			if (worldSessionId != sessionId)
			{
				otherSessionIds.push_back(worldSessionId);
			}
		}
		(void)StageSpawnAddPacketToSessions(
			_network,
			std::span<const SessionId>(otherSessionIds),
			spawnEvent.netId,
			characterId,
			transform);
	}
}
void ServerApp::BuildReplication()
{
	std::vector<SessionId> worldSessionIds;
	for (WorldId worldId : _framework.GetRunnableWorldIds())
	{
		WorldInstance* const world = _framework.FindWorld(worldId);
		if (world == nullptr)
		{
			continue;
		}

		_sessionBindings.CollectSessionsInWorld(worldId, worldSessionIds);
		if (worldSessionIds.empty())
		{
			continue;
		}

		ECSView view = world->GetRuntime().MakeView();
		for (auto [entity, dirty] : view.View<DirtyFlagsComp>())
		{
			if (!dirty.AnyDirty() || !view.HasComponent<ReplicatedTag>(entity))
			{
				continue;
			}

			const NetId netId = _framework.FindNetId(worldId, entity);
			if (!netId.IsValid())
			{
				dirty.Clear();
				continue;
			}

			if (dirty.IsDirty(WorldDirtyType::Transform))
			{
				const WorldTransformComp* transform =
					view.GetComponent<WorldTransformComp>(entity);
				if (transform != nullptr)
				{
					Protocol::SC_MOVE_PACKET movePacket;
					movePacket.set_netid(netId.GetRaw());
					movePacket.set_x(transform->position.x);
					movePacket.set_y(transform->position.y);
					movePacket.set_z(transform->position.z);
					movePacket.set_yaw(
						TransformHelper::QuaternionToYaw(transform->rotation));
					(void)StageReplicationPacket(
						_network,
						PacketType::SC_MOVE_OBJECT,
						worldSessionIds,
						movePacket);
				}
			}

			if (dirty.IsDirty(WorldDirtyType::Animation))
			{
				const AnimationPlaybackStateComp* playback =
					view.GetComponent<AnimationPlaybackStateComp>(entity);
				if (playback != nullptr &&
					playback->animationId != AnimationId::None)
				{
					Protocol::SC_ANIMATION_TRANSITION_PACKET animationPacket;
					animationPacket.set_netid(netId.GetRaw());
					animationPacket.set_curranim(
						static_cast<int32_t>(playback->animationId));
					(void)StageReplicationPacket(
						_network,
						PacketType::SC_ANIMATION_CHANGE,
						worldSessionIds,
						animationPacket);
				}
			}

			if (dirty.IsDirty(WorldDirtyType::Stat))
			{
				const CombatStatStateComp* stats =
					view.GetComponent<CombatStatStateComp>(entity);
				const SessionId ownerSessionId =
					_sessionBindings.FindOwnerSession(netId);
				if (stats != nullptr && ownerSessionId != 0)
				{
					Protocol::SC_STAT_CHANGE_PACKET statPacket;
					statPacket.set_netid(netId.GetRaw());
					statPacket.set_curhp(
						static_cast<uint32_t>(std::max(0, stats->currentHp)));
					statPacket.set_maxhp(
						static_cast<uint32_t>(std::max(0, stats->maxHp)));
					statPacket.set_curstamina(
						static_cast<uint32_t>(std::max(0, stats->currentStamina)));
					statPacket.set_maxstamina(
						static_cast<uint32_t>(std::max(0, stats->maxStamina)));
					statPacket.set_power(
						static_cast<uint32_t>(std::max(0, stats->attackPower)));
					statPacket.set_attackspeed(stats->attackSpeed);
					statPacket.set_defense(
						static_cast<uint32_t>(std::max(0, stats->defense)));
					statPacket.set_movespeed(
						static_cast<uint32_t>(
							std::max(0.0f, stats->moveSpeed)));
					(void)StageReplicationPacket(
						_network,
						PacketType::SC_STAT_CHANGE,
						std::span<const SessionId>(&ownerSessionId, 1),
						statPacket);
				}
			}

			dirty.Clear();
		}
	}
}

void ServerApp::FlushOutbound()
{
	_network.FlushSendStage();
}
