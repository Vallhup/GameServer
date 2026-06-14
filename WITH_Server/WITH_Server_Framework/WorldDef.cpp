#include "pch.h"
#include "WorldDef.h"

WorldDef CreatePlazaWorldDef(WorldExecutionModelKey executionModelKey)
{
	return WorldDef
	{
		.id = WorldDefId::Plaza,
		.name = "Plaza",

		.topology = WorldTopologyDef{
			.kind = WorldKind::Hub,
			.instanceType = WorldInstanceType::Persistent,
		},

		.entryPolicy = WorldEntryPolicyDef{
			.creationPolicy = CreationPolicy::PreCreated,
			.joinPolicy = JoinPolicy::FreeJoin,
			.maxPlayerCount = 5000,
			.allowReEntry = false,
			.destroyWhenEmpty = false,
			.emptyDestroyDelaySec = std::nullopt,
			.fallbackWorldDefId = std::nullopt,
		},

		.map = MapDef{
			.resourceId = 1,
			.defaultPlayerSpawnPointId = SpawnPointIds::PlazaPlayerStart,
			.spawnPoints = {
				SpawnPointDef
				{
					.id = SpawnPointIds::PlazaPlayerStart,
					.name = "Plaza.PlayerStart",
					.position = WorldVec3Def{ 502.362610f, 5.600996f, 482.074982f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				}
			},
			// TODO(content): replace temporary navmesh path with Plaza map data.
			.navMesh = MapNavMeshDef
			{
				.navMeshBinPath = "../Map/Plaza_NavMesh_v4.bin",
				.agentRadius = kUnityNavMeshAgentRadius,
				.agentHeight = kUnityNavMeshAgentHeight,
				.agentMaxClimb = kUnityNavMeshAgentMaxClimb,
				.agentMaxSlope = kUnityNavMeshAgentMaxSlope
			},
			.terrainHeight = TerrainHeightRawDef
			{
				.path = "../Map/Plaza_Terrain.raw",
				.width = 2049,
				.height = 2049,
				.originX = 0.0f,
				.originZ = 0.0f,
				.rotationYDegrees = 0.0f,
				.cellSizeX = 1016.0f / 2048.0f,
				.cellSizeZ = 1016.0f / 2048.0f,
				.heightScale = 27.01563f / 65535.0f,
				.heightOffset = 0.0f,
				.flipZ = false,
				.sampleFormat = TerrainHeightSampleFormat::UInt16LE,
			},
			.navigationProfile = NavigationProfileDef
			{
				.id = 0,
				.nearestPolyExtentXZ = 2.0f,
				.nearestPolyExtentY = 4.0f,
				.navMeshSurfaceYOffset = -0.25f,
				.queryFilter = NavigationQueryFilterDef
				{
					.walkableAreaCost = 1.0f,
					.includeFlags = 0xFFFF,
					.excludeFlags = 0
				}
			},
			.navigationProfileId = std::nullopt,
			.environmentTags = {},
		},

		.spawn = WorldSpawnDef
		{
			.initialSpawnSetId = SpawnSetId::PlazaDefault,
			.respawnSpawnSetId = std::nullopt,
		},

		.progressRule = WorldProgressRuleDef
		{
			.clearType = WorldClearConditionType::None,
			.failType = WorldFailConditionType::None,
			.completionType = WorldCompletionActionType::None,
			.completionDelaySec = std::nullopt,
			.autoCloseOnComplete = false,
		},

		.linkRules = {},
		.executionModelKey = executionModelKey,
		.transferProfileId = PlayerCharacterWorldTransferProfileId,
	};
}

WorldDef CreateVillageWorldDef(WorldExecutionModelKey executionModelKey)
{
	return WorldDef{
		.id = WorldDefId::Village,
		.name = "Village",

		.topology = WorldTopologyDef
		{
			.kind = WorldKind::Dungeon,
			.instanceType = WorldInstanceType::Instanced,
		},

		.entryPolicy = WorldEntryPolicyDef
		{
			.creationPolicy = CreationPolicy::CreateOnDemand,
			.joinPolicy = JoinPolicy::PartyOnly,
			.maxPlayerCount = 3,
			.allowReEntry = false,
			.destroyWhenEmpty = true,
			.emptyDestroyDelaySec = 5.0f,
			.fallbackWorldDefId = WorldDefId::Plaza,
		},

		.map = MapDef
		{
			.resourceId = 2,
			.defaultPlayerSpawnPointId = SpawnPointIds::VillagePlayerStart,
			.spawnPoints = {
				SpawnPointDef{
					.id = SpawnPointIds::VillagePlayerStart,
					.name = "Village.PlayerStart",
					.position = WorldVec3Def{ 159.286636f, 48.944988f, 649.457764f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::VillageMonster01,
					.name = "Village.Imp.01",
					.position = WorldVec3Def{ 212.904114f, 56.205055f, 620.986938f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::VillageMonster01A,
					.name = "Village.Imp.01A",
					.position = WorldVec3Def{ 214.304108f, 56.205055f, 621.786926f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::VillageMonster01B,
					.name = "Village.Imp.01B",
					.position = WorldVec3Def{ 211.704117f, 56.205055f, 622.286926f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::VillageMonster02,
					.name = "Village.DemonExecutioner.02",
					.position = WorldVec3Def{ 216.599396f, 56.203743f, 579.089844f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::VillageMonster03,
					.name = "Village.DemonStriker.01",
					.position = WorldVec3Def{ 263.797272f, 58.984131f, 556.220764f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::VillageMonster04,
					.name = "Village.DemonExecutioner.01",
					.position = WorldVec3Def{ 272.319183f, 62.978153f, 651.002991f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::VillageMonster05,
					.name = "Village.Imp.03",
					.position = WorldVec3Def{ 240.033142f, 57.021049f, 606.717407f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::VillageMonster05A,
					.name = "Village.Imp.03A",
					.position = WorldVec3Def{ 241.433136f, 57.021049f, 607.517395f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::VillageMonster05B,
					.name = "Village.Imp.03B",
					.position = WorldVec3Def{ 238.833145f, 57.021049f, 608.017395f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::VillageMonster06,
					.name = "Village.Imp.04",
					.position = WorldVec3Def{ 186.963013f, 56.395004f, 583.231812f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::VillageMonster06A,
					.name = "Village.Imp.04A",
					.position = WorldVec3Def{ 188.363007f, 56.395004f, 584.031799f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::VillageMonster06B,
					.name = "Village.Imp.04B",
					.position = WorldVec3Def{ 185.763016f, 56.395004f, 584.531799f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::VillageMonster07,
					.name = "Village.DemonStriker.02",
					.position = WorldVec3Def{ 263.688568f, 62.369606f, 620.089600f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::VillageMonster08,
					.name = "Village.Imp.05",
					.position = WorldVec3Def{ 287.081970f, 65.820259f, 639.837524f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::VillageMonster08A,
					.name = "Village.Imp.05A",
					.position = WorldVec3Def{ 288.481964f, 65.820259f, 640.637512f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::VillageMonster09,
					.name = "Village.DemonStriker.03",
					.position = WorldVec3Def{ 239.990372f, 56.680523f, 540.022034f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::VillageMonster10,
					.name = "Village.DemonExecutioner.03",
					.position = WorldVec3Def{ 292.899323f, 68.393768f, 570.101746f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::VillageBossMonster01,
					.name = "Village.Boss.01",
					.position = WorldVec3Def{ 345.541107f, 75.819321f, 587.426025f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
			},
			.navMesh = MapNavMeshDef
			{
				.navMeshBinPath = "../Map/Village_NavMesh_v13.bin",
				.agentRadius = kUnityNavMeshAgentRadius,
				.agentHeight = kUnityNavMeshAgentHeight,
				.agentMaxClimb = kUnityNavMeshAgentMaxClimb,
				.agentMaxSlope = kUnityNavMeshAgentMaxSlope
			},
			.terrainHeight = TerrainHeightRawDef
			{
				.path = "../Map/Village_Terrain.raw",
				.width = 2049,
				.height = 2049,
				.originX = 0.0f,
				.originZ = 0.0f,
				.rotationYDegrees = 90.0f,
				.cellSizeX = 1023.0f / 2048.0f,
				.cellSizeZ = 1023.0f / 2048.0f,
				.heightScale = 159.4766f / 65535.0f,
				.heightOffset = 0.0f,
				.flipZ = false,
				.sampleFormat = TerrainHeightSampleFormat::UInt16LE,
			},
			.navigationProfile = NavigationProfileDef
			{
				.id = 0,
				.nearestPolyExtentXZ = 2.0f,
				.nearestPolyExtentY = 4.0f,
				.navMeshSurfaceYOffset = 0.0f,
				.queryFilter = NavigationQueryFilterDef
				{
					.walkableAreaCost = 1.0f,
					.includeFlags = 0xFFFF,
					.excludeFlags = 0
				}
			},
			.navigationProfileId = std::nullopt,
			.environmentTags = {},
		},

		.spawn = WorldSpawnDef{
			.initialSpawnSetId = SpawnSetId::VillageDefault,
			.respawnSpawnSetId = std::nullopt,
		},

		.progressRule = WorldProgressRuleDef{
			.clearType = WorldClearConditionType::DefeatAllEnemies,
			.failType = WorldFailConditionType::AllPlayersDead,
			.completionType = WorldCompletionActionType::MoveToLinkedWorld,
			.completionDelaySec = std::nullopt,
			.autoCloseOnComplete = true,
		},

		.linkRules = {
			WorldLinkRuleDef{
				.linkType = WorldLinkType::ClearReward,
				.targetWorldDefId = WorldDefId::Castle,
				.linkConditionType = WorldLinkConditionType::OnClear,
				.numericConditionParameter = std::nullopt,
				.fallbackWorldDefId = WorldDefId::Plaza,
				.spawnPointId = SpawnPointIds::CastlePlayerStart,
			},
		},
		.executionModelKey = executionModelKey,
		.transferProfileId = PlayerCharacterWorldTransferProfileId,
	};
}

WorldDef CreateCastleWorldDef(WorldExecutionModelKey executionModelKey)
{
	return WorldDef{
		.id = WorldDefId::Castle,
		.name = "Castle",

		.topology = WorldTopologyDef{
			.kind = WorldKind::Dungeon,
			.instanceType = WorldInstanceType::Instanced,
		},

		.entryPolicy = WorldEntryPolicyDef{
			.creationPolicy = CreationPolicy::CreateOnDemand,
			.joinPolicy = JoinPolicy::PartyOnly,
			.maxPlayerCount = 3,
			.allowReEntry = false,
			.destroyWhenEmpty = true,
			.emptyDestroyDelaySec = 5.0f,
			.fallbackWorldDefId = WorldDefId::Plaza,
		},

		.map = MapDef{
			.resourceId = 3,
			.defaultPlayerSpawnPointId = SpawnPointIds::CastlePlayerStart,
			.spawnPoints = {
				SpawnPointDef{
					.id = SpawnPointIds::CastlePlayerStart,
					.name = "Castle.PlayerStart",
					.position = WorldVec3Def{ 325.256653f, 67.439247f, 206.893997f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::CastleMonster01,
					.name = "Castle.Imp.01",
					.position = WorldVec3Def{ 326.687042f, 69.436005f, 326.435638f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::CastleMonster01A,
					.name = "Castle.Imp.01A",
					.position = WorldVec3Def{ 328.087036f, 69.436005f, 327.235626f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::CastleMonster01B,
					.name = "Castle.Imp.01B",
					.position = WorldVec3Def{ 325.487030f, 69.436005f, 327.735626f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::CastleMonster02,
					.name = "Castle.DemonStriker.01",
					.position = WorldVec3Def{ 317.817505f, 69.147423f, 350.333435f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::CastleMonster03,
					.name = "Castle.Imp.02",
					.position = WorldVec3Def{ 372.233459f, 70.180191f, 359.069763f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::CastleMonster03A,
					.name = "Castle.Imp.02A",
					.position = WorldVec3Def{ 373.633453f, 70.180191f, 359.869751f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::CastleMonster03B,
					.name = "Castle.Imp.02B",
					.position = WorldVec3Def{ 371.033447f, 70.180191f, 360.369751f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::CastleMonster04,
					.name = "Castle.DemonExecutioner.01",
					.position = WorldVec3Def{ 363.646759f, 68.896736f, 375.680389f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::CastleMonster05,
					.name = "Castle.Tank.01",
					.position = WorldVec3Def{ 338.563477f, 69.991455f, 425.379242f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::CastleMonster06,
					.name = "Castle.Imp.03",
					.position = WorldVec3Def{ 308.561279f, 67.742950f, 382.392914f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::CastleMonster07,
					.name = "Castle.Imp.04",
					.position = WorldVec3Def{ 353.739288f, 68.879623f, 308.454926f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::CastleMonster07A,
					.name = "Castle.Imp.04A",
					.position = WorldVec3Def{ 355.139282f, 68.879623f, 309.254913f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::CastleMonster07B,
					.name = "Castle.Imp.04B",
					.position = WorldVec3Def{ 352.539276f, 68.879623f, 309.754913f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::CastleMonster08,
					.name = "Castle.DemonExecutioner.02",
					.position = WorldVec3Def{ 367.923859f, 68.897438f, 328.193909f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::CastleMonster09,
					.name = "Castle.DemonStriker.02",
					.position = WorldVec3Def{ 353.965179f, 68.888054f, 333.697205f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::CastleMonster10,
					.name = "Castle.DemonExecutioner.03",
					.position = WorldVec3Def{ 311.321808f, 69.173592f, 321.829773f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::CastleMonster11,
					.name = "Castle.DemonStriker.03",
					.position = WorldVec3Def{ 344.273621f, 68.230255f, 383.559265f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::CastleMonster12,
					.name = "Castle.Imp.05",
					.position = WorldVec3Def{ 318.663025f, 67.961067f, 371.518890f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::CastleMonster12A,
					.name = "Castle.Imp.05A",
					.position = WorldVec3Def{ 320.063019f, 67.961067f, 372.318878f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::CastleMonster12B,
					.name = "Castle.Imp.05B",
					.position = WorldVec3Def{ 317.463013f, 67.961067f, 372.818878f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
			},
			.navMesh = MapNavMeshDef
			{
				.navMeshBinPath = "../Map/Castle_NavMesh_v7.bin",
				.agentRadius = kUnityNavMeshAgentRadius,
				.agentHeight = kUnityNavMeshAgentHeight,
				.agentMaxClimb = kUnityNavMeshAgentMaxClimb,
				.agentMaxSlope = kUnityNavMeshAgentMaxSlope
			},
			.terrainHeight = TerrainHeightRawDef
			{
				.path = "../Map/Castle_Terrain.raw",
				.width = 2049,
				.height = 2049,
				.originX = 0.0f,
				.originZ = 0.0f,
				.rotationYDegrees = 0.0f,
				.cellSizeX = 650.2402f / 2048.0f,
				.cellSizeZ = 650.2402f / 2048.0f,
				.heightScale = 79.28662f / 65535.0f,
				.heightOffset = 0.0f,
				.flipZ = false,
				.sampleFormat = TerrainHeightSampleFormat::UInt16LE,
			},
			.navigationProfile = NavigationProfileDef
			{
				.id = 0,
				.nearestPolyExtentXZ = 2.0f,
				.nearestPolyExtentY = 4.0f,
				.navMeshSurfaceYOffset = 0.0f,
				.queryFilter = NavigationQueryFilterDef
				{
					.walkableAreaCost = 1.0f,
					.includeFlags = 0xFFFF,
					.excludeFlags = 0
				}
			},
			.navigationProfileId = std::nullopt,
			.environmentTags = {},
		},

		.spawn = WorldSpawnDef{
			.initialSpawnSetId = SpawnSetId::CastleDefault,
			.respawnSpawnSetId = std::nullopt,
		},

		.progressRule = WorldProgressRuleDef{
			.clearType = WorldClearConditionType::DefeatAllEnemies,
			.failType = WorldFailConditionType::AllPlayersDead,
			.completionType = WorldCompletionActionType::MoveToLinkedWorld,
			.completionDelaySec = std::nullopt,
			.autoCloseOnComplete = true,
		},

		.linkRules = {
			WorldLinkRuleDef{
				.linkType = WorldLinkType::ClearReward,
				.targetWorldDefId = WorldDefId::Final,
				.linkConditionType = WorldLinkConditionType::OnClear,
				.numericConditionParameter = std::nullopt,
				.fallbackWorldDefId = WorldDefId::Plaza,
				.spawnPointId = SpawnPointIds::FinalPlayerStart,
			},
		},
		.executionModelKey = executionModelKey,
		.transferProfileId = PlayerCharacterWorldTransferProfileId,
	};
}

WorldDef CreateFinalWorldDef(WorldExecutionModelKey executionModelKey)
{
	return WorldDef{
		.id = WorldDefId::Final,
		.name = "Final",

		.topology = WorldTopologyDef{
			.kind = WorldKind::Dungeon,
			.instanceType = WorldInstanceType::Instanced,
		},

		.entryPolicy = WorldEntryPolicyDef{
			.creationPolicy = CreationPolicy::CreateOnDemand,
			.joinPolicy = JoinPolicy::PartyOnly,
			.maxPlayerCount = 3,
			.allowReEntry = false,
			.destroyWhenEmpty = true,
			.emptyDestroyDelaySec = 5.0f,
			.fallbackWorldDefId = WorldDefId::Plaza,
		},

		.map = MapDef{
			.resourceId = 4,
			.defaultPlayerSpawnPointId = SpawnPointIds::FinalPlayerStart,
			.spawnPoints = {
				SpawnPointDef{
					.id = SpawnPointIds::FinalPlayerStart,
					.name = "Final.PlayerStart",
					.position = WorldVec3Def{ 0.0f, 5.0f, -96.193400f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::FinalMonster01,
					.name = "Final.FinalBoss.01",
					.position = WorldVec3Def{ 0.044109f, 2.180071f, 2.412330f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
			},
			.navMesh = MapNavMeshDef
			{
				.navMeshBinPath = "../Map/Cathedral_NavMesh_v4.bin",
				.agentRadius = kUnityNavMeshAgentRadius,
				.agentHeight = kUnityNavMeshAgentHeight,
				.agentMaxClimb = kUnityNavMeshAgentMaxClimb,
				.agentMaxSlope = kUnityNavMeshAgentMaxSlope
			},
			.terrainHeight = TerrainHeightRawDef
			{
				.path = "../Map/Cathedral_Terrain.raw",
				.width = 2049,
				.height = 2049,
				.originX = -57.9f,
				.originZ = -102.2f,
				.rotationYDegrees = 0.0f,
				.cellSizeX = 120.0f / 2048.0f,
				.cellSizeZ = 120.0f / 2048.0f,
				.heightScale = 600.0f / 65535.0f,
				.heightOffset = 0.0f,
				.flipZ = false,
				.sampleFormat = TerrainHeightSampleFormat::UInt16LE,
			},
			.navigationProfile = NavigationProfileDef
			{
				.id = 0,
				.nearestPolyExtentXZ = 2.0f,
				.nearestPolyExtentY = 4.0f,
				.navMeshSurfaceYOffset = 0.0f,
				.queryFilter = NavigationQueryFilterDef
				{
					.walkableAreaCost = 1.0f,
					.includeFlags = 0xFFFF,
					.excludeFlags = 0
				}
			},
			.navigationProfileId = std::nullopt,
			.environmentTags = {},
		},

		.spawn = WorldSpawnDef{
			.initialSpawnSetId = SpawnSetId::FinalDefault,
			.respawnSpawnSetId = std::nullopt,
		},

		.progressRule = WorldProgressRuleDef{
			.clearType = WorldClearConditionType::DefeatAllEnemies,
			.failType = WorldFailConditionType::AllPlayersDead,
			.completionType = WorldCompletionActionType::None,
			.completionDelaySec = std::nullopt,
			.autoCloseOnComplete = false,
		},
		.executionModelKey = executionModelKey,
		.transferProfileId = PlayerCharacterWorldTransferProfileId,
	};
}

WorldDef CreatePvpWorldDef(WorldExecutionModelKey executionModelKey)
{
	return WorldDef{
		.id = WorldDefId::Pvp,
		.name = "Pvp",

		.topology = WorldTopologyDef{
			.kind = WorldKind::Field,
			.instanceType = WorldInstanceType::Instanced,
		},

		.entryPolicy = WorldEntryPolicyDef{
			.creationPolicy = CreationPolicy::CreateOnDemand,
			.joinPolicy = JoinPolicy::PartyOnly,
			.maxPlayerCount = 3,
			.allowReEntry = false,
			.destroyWhenEmpty = true,
			.emptyDestroyDelaySec = 5.0f,
			.fallbackWorldDefId = WorldDefId::Plaza,
		},

		.map = MapDef{
			.resourceId = 4,
			.defaultPlayerSpawnPointId = SpawnPointIds::PvpPlayerStartA,
			.spawnPoints = {
				SpawnPointDef{
					.id = SpawnPointIds::PvpPlayerStartA,
					.name = "Pvp.PlayerStart.A",
					.position = WorldVec3Def{ -5.534470f, 0.128176f, -26.368433f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::PvpPlayerStartB,
					.name = "Pvp.PlayerStart.B",
					.position = WorldVec3Def{ 5.679338f, 0.128176f, -26.370371f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::PvpPlayerStartC,
					.name = "Pvp.PlayerStart.C",
					.position = WorldVec3Def{ -0.068215f, 0.146487f, -12.604228f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
			},
			.navMesh = MapNavMeshDef
			{
				.navMeshBinPath = "../Map/Cathedral_NavMesh_v4.bin",
				.agentRadius = kUnityNavMeshAgentRadius,
				.agentHeight = kUnityNavMeshAgentHeight,
				.agentMaxClimb = kUnityNavMeshAgentMaxClimb,
				.agentMaxSlope = kUnityNavMeshAgentMaxSlope
			},
			.terrainHeight = TerrainHeightRawDef
			{
				.path = "../Map/Cathedral_Terrain.raw",
				.width = 2049,
				.height = 2049,
				.originX = -57.9f,
				.originZ = -102.2f,
				.rotationYDegrees = 0.0f,
				.cellSizeX = 120.0f / 2048.0f,
				.cellSizeZ = 120.0f / 2048.0f,
				.heightScale = 600.0f / 65535.0f,
				.heightOffset = 0.0f,
				.flipZ = false,
				.sampleFormat = TerrainHeightSampleFormat::UInt16LE,
			},
			.navigationProfile = NavigationProfileDef
			{
				.id = 0,
				.nearestPolyExtentXZ = 2.0f,
				.nearestPolyExtentY = 4.0f,
				.navMeshSurfaceYOffset = 0.0f,
				.queryFilter = NavigationQueryFilterDef
				{
					.walkableAreaCost = 1.0f,
					.includeFlags = 0xFFFF,
					.excludeFlags = 0
				}
			},
			.navigationProfileId = std::nullopt,
			.environmentTags = {},
		},

		.spawn = WorldSpawnDef{
			.initialSpawnSetId = SpawnSetId::PvpDefault,
			.respawnSpawnSetId = std::nullopt,
		},

		.progressRule = WorldProgressRuleDef{
			.clearType = WorldClearConditionType::None,
			.failType = WorldFailConditionType::None,
			.completionType = WorldCompletionActionType::None,
			.completionDelaySec = std::nullopt,
			.autoCloseOnComplete = false,
		},
		.linkRules = {},
		.executionModelKey = executionModelKey,
		.transferProfileId = PlayerCharacterWorldTransferProfileId,
	};
}
