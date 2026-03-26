#pragma once

#include "IDs.h"
#include <string>
#include <optional>

enum class WorldKind : uint8_t
{
	Hub,
	Dungeon,
	Filed
};

enum class InstanceType : uint8_t
{
	Persistent,
	Instanced,
	SessionScoped
};

struct WorldTopologyDef
{
	WorldKind kind;
	InstanceType type;
};

enum class CreationPolicy : uint8_t
{
	PreCreated,
	CreateOnDemand
};

enum class JoinPolicy : uint8_t
{
	FreeJoin,
	PartyOnly,
	MatchmakingOnly
};

struct WorldEntryPolicyDef
{
	CreationPolicy creationPolicy;
	JoinPolicy joinPolicy;
	uint16_t maxPlayerCount;
	bool allowReEntry;
	bool destroyWhenEmpty;
	std::optional<float> emptyDestroyDelaySec;
	std::optional<WorldId> fallbackWorldId;
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
	MapResourceId id;
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

using EntryPointId = uint16_t;

struct WorldLinkRuleDef
{
	WorldLinkType linkType;
	WorldId targetWorldId;

	WorldLinkConditionType linkConditionType;
	std::optional<float> conditionParameter;

	std::optional<WorldId> fallbackWorldId;
	std::optional<EntryPointId> entryPoindId;
};

struct WorldDef
{
	WorldId id;
	std::string name;

	WorldTopologyDef topology;
	WorldEntryPolicyDef entryPolicy;

	MapDef map;
	WorldSpawnDef spawn;
	WorldProgressRuleDef progressRule;
	
	std::vector<WorldLinkRuleDef> linkRules;
};