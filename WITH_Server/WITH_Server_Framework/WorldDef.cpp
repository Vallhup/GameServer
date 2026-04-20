#include "pch.h"
#include "WorldDef.h"

namespace
{
	constexpr uint16_t kPlazaMaxPlayers = 5000;
	constexpr uint16_t kPartyMaxPlayers = 3;
	constexpr float kCombatEmptyDestroyDelaySec = 5.0f;

	constexpr MapResourceId kPlazaMapResourceId = 1;
	constexpr MapResourceId kVillageMapResourceId = 2;
	constexpr MapResourceId kCastleMapResourceId = 3;
	constexpr MapResourceId kFinalMapResourceId = 4;
	constexpr MapResourceId kPvpMapResourceId = 5;

	class WorldDefBuilder final {
	public:
		static WorldDef CreateBase(
			WorldDefId id,
			const char* name,
			WorldKind kind,
			WorldInstanceType instanceType,
			WorldExecutionModelKey executionModelKey)
		{
			WorldDef def{};
			def.id = id;
			def.name = name;
			def.topology.kind = kind;
			def.topology.instanceType = instanceType;
			def.executionModelKey = executionModelKey;
			def.transferProfileId = PlayerCharacterWorldTransferProfileId;

			ApplyUnresolvedMapDefaults(def);
			def.linkRules.clear();
			return def;
		}

		static void ApplyNoCompletionProgress(WorldDef& def)
		{
			def.progressRule.clearType = WorldClearConditionType::None;
			def.progressRule.failType = WorldFailConditionType::None;
			def.progressRule.completionType = WorldCompletionActionType::None;
			def.progressRule.completionDelaySec = std::nullopt;
			def.progressRule.autoCloseOnComplete = false;
		}

		static void ApplyCombatEntryPolicy(WorldDef& def)
		{
			def.entryPolicy.creationPolicy = CreationPolicy::CreateOnDemand;
			def.entryPolicy.joinPolicy = JoinPolicy::PartyOnly;
			def.entryPolicy.maxPlayerCount = kPartyMaxPlayers;
			def.entryPolicy.allowReEntry = false;
			def.entryPolicy.destroyWhenEmpty = true;
			def.entryPolicy.emptyDestroyDelaySec = kCombatEmptyDestroyDelaySec;
			def.entryPolicy.fallbackWorldDefId = WorldDefId::Plaza;
		}

		static void ApplySequentialPveProgress(
			WorldDef& def,
			WorldDefId nextWorldDefId)
		{
			def.progressRule.clearType = WorldClearConditionType::DefeatAllEnemies;
			def.progressRule.failType = WorldFailConditionType::AllPlayersDead;
			def.progressRule.completionType = WorldCompletionActionType::MoveToLinkedWorld;
			def.progressRule.completionDelaySec = std::nullopt;
			def.progressRule.autoCloseOnComplete = true;

			WorldLinkRuleDef link{};
			link.linkType = WorldLinkType::ClearReward;
			link.targetWorldDefId = nextWorldDefId;
			link.linkConditionType = WorldLinkConditionType::OnClear;
			link.numericConditionParameter = std::nullopt;
			link.fallbackWorldDefId = WorldDefId::Plaza;
			link.spawnPointId = std::nullopt;
			def.linkRules.push_back(link);
		}

	private:
		static void ApplyUnresolvedMapDefaults(WorldDef& def)
		{
			// TODO: Fill actual map resource, spawn points, named spawn points,
			// navmesh path, navigation profile, and environment tags per world.
			def.map.resourceId = ResolveMapResourceId(def.id);
			def.map.defaultPlayerSpawnPointId = 0;
			def.map.namedSpawnPoints.clear();
			def.map.navMesh = std::nullopt;
			def.map.navigationProfile = std::nullopt;
			def.map.navigationProfileId = std::nullopt;
			def.map.environmentTags.clear();
		}

		static MapResourceId ResolveMapResourceId(WorldDefId worldDefId) noexcept
		{
			switch (worldDefId) {
			case WorldDefId::Plaza:
				return kPlazaMapResourceId;
			case WorldDefId::Village:
				return kVillageMapResourceId;
			case WorldDefId::Castle:
				return kCastleMapResourceId;
			case WorldDefId::Final:
				return kFinalMapResourceId;
			case WorldDefId::Pvp:
				return kPvpMapResourceId;
			default:
				return 0;
			}
		}
	};
}

WorldDef CreatePlazaWorldDef(WorldExecutionModelKey executionModelKey)
{
	WorldDef def = WorldDefBuilder::CreateBase(
		WorldDefId::Plaza,
		"Plaza",
		WorldKind::Hub,
		WorldInstanceType::Persistent,
		executionModelKey);

	// TEMP
	def.map.navMesh = MapNavMeshDef
	{ 
		.navMeshBinPath = "../Map/Cathedral_NavMesh_v2.bin" 
	};

	def.map.navigationProfile = NavigationProfileDef
	{
		.id = 1,
		.nearestPolyExtentXZ = 2.0f,
		.nearestPolyExtentY = 4.0f,
		.navMeshSurfaceYOffset = 0.0f,
		.queryFilter = NavigationQueryFilterDef
		{
			.walkableAreaCost = 1.0f,
			.includeFlags = 0xFFFF,
			.excludeFlags = 0,
		},
	};

	def.entryPolicy.creationPolicy = CreationPolicy::PreCreated;
	def.entryPolicy.joinPolicy = JoinPolicy::FreeJoin;
	def.entryPolicy.maxPlayerCount = kPlazaMaxPlayers;
	def.entryPolicy.allowReEntry = false;
	def.entryPolicy.destroyWhenEmpty = false;
	def.entryPolicy.emptyDestroyDelaySec = std::nullopt;
	def.entryPolicy.fallbackWorldDefId = std::nullopt;

	def.spawn.initialSpawnSetId = SpawnSetId::PlazaDefault;
	def.spawn.respawnSpawnSetId = std::nullopt;

	WorldDefBuilder::ApplyNoCompletionProgress(def);
	return def;
}

WorldDef CreateVillageWorldDef(WorldExecutionModelKey executionModelKey)
{
	WorldDef def = WorldDefBuilder::CreateBase(
		WorldDefId::Village,
		"Village",
		WorldKind::Dungeon,
		WorldInstanceType::Instanced,
		executionModelKey);

	WorldDefBuilder::ApplyCombatEntryPolicy(def);
	def.spawn.initialSpawnSetId = SpawnSetId::VillageDefault;
	def.spawn.respawnSpawnSetId = std::nullopt;
	WorldDefBuilder::ApplySequentialPveProgress(def, WorldDefId::Castle);
	return def;
}

WorldDef CreateCastleWorldDef(WorldExecutionModelKey executionModelKey)
{
	WorldDef def = WorldDefBuilder::CreateBase(
		WorldDefId::Castle,
		"Castle",
		WorldKind::Dungeon,
		WorldInstanceType::Instanced,
		executionModelKey);

	WorldDefBuilder::ApplyCombatEntryPolicy(def);
	def.spawn.initialSpawnSetId = SpawnSetId::CastleDefault;
	def.spawn.respawnSpawnSetId = std::nullopt;
	WorldDefBuilder::ApplySequentialPveProgress(def, WorldDefId::Final);
	return def;
}

WorldDef CreateFinalWorldDef(WorldExecutionModelKey executionModelKey)
{
	WorldDef def = WorldDefBuilder::CreateBase(
		WorldDefId::Final,
		"Final",
		WorldKind::Dungeon,
		WorldInstanceType::Instanced,
		executionModelKey);

	WorldDefBuilder::ApplyCombatEntryPolicy(def);
	def.spawn.initialSpawnSetId = SpawnSetId::FinalDefault;
	def.spawn.respawnSpawnSetId = std::nullopt;

	def.progressRule.clearType = WorldClearConditionType::DefeatAllEnemies;
	def.progressRule.failType = WorldFailConditionType::AllPlayersDead;
	def.progressRule.completionType = WorldCompletionActionType::None;
	def.progressRule.completionDelaySec = std::nullopt;
	def.progressRule.autoCloseOnComplete = false;

	// TODO: Add scripted Plaza/Pvp choice metadata once the Final clear UI
	// request path is defined.
	return def;
}

WorldDef CreatePvpWorldDef(WorldExecutionModelKey executionModelKey)
{
	WorldDef def = WorldDefBuilder::CreateBase(
		WorldDefId::Pvp,
		"Pvp",
		WorldKind::Field,
		WorldInstanceType::Instanced,
		executionModelKey);

	WorldDefBuilder::ApplyCombatEntryPolicy(def);
	def.spawn.initialSpawnSetId = SpawnSetId::PvpDefault;
	def.spawn.respawnSpawnSetId = std::nullopt;
	WorldDefBuilder::ApplyNoCompletionProgress(def);

	// TODO: Add WorldKind::Arena and PvP-specific clear/fail rules when the
	// 1-party internal PvP rule system is defined.
	return def;
}
