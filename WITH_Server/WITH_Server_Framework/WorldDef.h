#pragma once

#include "WorldContentIds.h"

#include <vector>
#include <string>
#include <optional>

enum class WorldKind : uint8_t
{
	Hub,
	Dungeon,
	Field
};

enum class WorldInstanceType : uint8_t
{
	Persistent,
	Instanced,
	SessionScoped
};

struct WorldTopologyDef
{
	WorldKind kind;
	WorldInstanceType instanceType;
};

enum class CreationPolicy : uint8_t
{
	PreCreated,
	CreateOnDemand
};

enum class JoinPolicy : uint8_t
{
	FreeJoin,
	PartyOnly
};

struct WorldEntryPolicyDef
{
	CreationPolicy creationPolicy;
	JoinPolicy joinPolicy;
	uint16_t maxPlayerCount;
	bool allowReEntry;
	bool destroyWhenEmpty;
	std::optional<float> emptyDestroyDelaySec;
	std::optional<WorldDefId> fallbackWorldDefId;
};

using MapResourceId = uint16_t;
using SpawnPointId = uint16_t;
using NavigationProfileId = uint16_t;
using EnvironmentTagId = uint16_t;

struct NamedSpawnPointDef
{
	SpawnPointId id;
	std::string name;
};

// NavMesh 파일 및 에이전트 물리 속성.
// agent* 값은 Recast 오프라인 빌드 시 사용한 파라미터와 반드시 일치해야 한다.
struct MapNavMeshDef
{
	std::string navMeshBinPath;
	float       agentRadius{ 0.35f }; // 0.6f
	float       agentHeight{ 2.0f };
	float       agentMaxClimb{ 0.4f }; // 0.9f
	float       agentMaxSlope{ 45.0f };
};

// Detour 쿼리 필터 파라미터
struct NavigationQueryFilterDef
{
	float          walkableAreaCost{ 1.0f };
	unsigned short includeFlags{ 0xFFFF };
	unsigned short excludeFlags{ 0 };
};

// NavMesh 쿼리 프로파일.
// nearestPolyExtent*: findNearestPoly/moveAlongSurface 검색 반경.
struct NavigationProfileDef
{
	NavigationProfileId      id{ 0 };
	float                    nearestPolyExtentXZ{ 2.0f };
	float                    nearestPolyExtentY{ 4.0f };
	float                    navMeshSurfaceYOffset{ 0.0f };
	NavigationQueryFilterDef queryFilter;
};

struct MapDef
{
	MapResourceId                       resourceId;
	SpawnPointId                        defaultPlayerSpawnPointId;
	std::vector<NamedSpawnPointDef>     namedSpawnPoints;
	std::optional<MapNavMeshDef>        navMesh;            // NavMesh 파일 정보 (없으면 NavMesh 미사용)
	std::optional<NavigationProfileDef> navigationProfile;  // 쿼리 파라미터 (navMesh 설정 시 함께 지정)
	std::optional<NavigationProfileId>  navigationProfileId; // 레거시 ID 필드 — 향후 제거 예정
	std::vector<EnvironmentTagId>       environmentTags;
};

struct WorldSpawnDef
{
	SpawnSetId initialSpawnSetId;
	std::optional<SpawnSetId> respawnSpawnSetId;
};

enum class WorldClearConditionType : uint8_t
{
	None,
	DefeatAllEnemies,
	ReachExit
};

enum class WorldFailConditionType : uint8_t
{
	None,
	AllPlayersDead,
	TimeExpired
};

enum class WorldCompletionActionType : uint8_t
{
	None,
	ReturnToFallbackWorld,
	MoveToLinkedWorld,
	CloseWorld
};

struct WorldProgressRuleDef
{
	WorldClearConditionType clearType;
	WorldFailConditionType failType;
	WorldCompletionActionType completionType;
	std::optional<float> completionDelaySec;
	bool autoCloseOnComplete;
};

enum class WorldLinkType : uint8_t
{
	Portal,
	ClearReward,
	Scripted
};

enum class WorldLinkConditionType : uint8_t
{
	Always,
	OnClear,
	OnFail,
	RequireItem
};

struct WorldLinkRuleDef
{
	WorldLinkType linkType;
	WorldDefId targetWorldDefId;

	WorldLinkConditionType linkConditionType;
	std::optional<float> numericConditionParameter;

	std::optional<WorldDefId> fallbackWorldDefId;
	std::optional<SpawnPointId> spawnPointId;
};

struct WorldDef
{
	WorldDefId id;
	std::string name;

	WorldTopologyDef topology;
	WorldEntryPolicyDef entryPolicy;

	MapDef map;
	WorldSpawnDef spawn;
	WorldProgressRuleDef progressRule;
	
	std::vector<WorldLinkRuleDef> linkRules;
	WorldExecutionModelKey executionModelKey{ InvalidWorldExecutionModelKey };
	WorldTransferProfileId transferProfileId{ InvalidWorldTransferProfileId };
};

// Framework 내장 월드 정의 팩토리.
// 실행 모델 키는 호출 측(서버/bootstrap)이 주입한다.
WorldDef CreatePlazaWorldDef(WorldExecutionModelKey executionModelKey);
WorldDef CreateVillageWorldDef(WorldExecutionModelKey executionModelKey);
WorldDef CreateCastleWorldDef(WorldExecutionModelKey executionModelKey);
WorldDef CreateFinalWorldDef(WorldExecutionModelKey executionModelKey);
WorldDef CreatePvpWorldDef(WorldExecutionModelKey executionModelKey);
