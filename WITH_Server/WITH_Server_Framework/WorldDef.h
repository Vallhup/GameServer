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

struct MapDef
{
	MapResourceId resourceId;
	SpawnPointId defaultPlayerSpawnPointId;
	std::vector<NamedSpawnPointDef> namedSpawnPoints;
	std::optional<NavigationProfileId> navigationProfileId;
	std::vector<EnvironmentTagId> environmentTags;
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
};