#pragma once

#include "GameplayComponentPrerequisites.h"

struct PendingDespawnTag : TagComponent
{
};

struct PlayerDeathCountConsumedTag : TagComponent
{
};

struct PendingMonsterKillEventComp : Component
{
	CharacterId killedCharacterId{};
	bool        pending{ false };
};

struct PendingPlayerDeathCountEventComp : Component
{
	// 사망 통계(DB) 소비용 1회성 플래그. RecordCombatStatisticsSystem 이 소비/초기화한다.
	bool        pending{ false };
	// 파티 데스카운트 결정(부활) 수확용 1회성 플래그. FrameworkFrameEventHarvester 가
	// 소비/초기화한다. pending 과 분리하여 통계 시스템이 부활 신호를 먼저 지우는 경합을 막는다.
	bool        decisionPending{ false };
	CharacterId killerCharacterId{};   // 몬스터에게 사망 시 세팅, 리스폰 시 초기화
};

enum class PlayerDeathState : uint8_t
{
	Alive = 0,
	WaitingForDeathCount,
	AwaitingRespawnInput,
	DeathCountExhausted
};

struct PlayerDeathStateComp : Component
{
	PlayerDeathState state{ PlayerDeathState::Alive };
	uint64_t deathCountRevision{ 0 };
	bool respawnRequested{ false };
};

struct PendingWorldTransferTag : TagComponent
{
};

struct PendingWorldTransferComp : Component
{
	WorldDefId targetWorldDefId{ WorldDefId::None };
	uint64_t instanceKey{ 0 };
	SpawnPointId spawnPointId{ 0 };
	bool hasSpawnPointId{ false };
	bool allowFallback{ false };
	uint16_t sourceTriggerId{ 0 };
	uint64_t requestedFrameIndex{ 0 };
};
