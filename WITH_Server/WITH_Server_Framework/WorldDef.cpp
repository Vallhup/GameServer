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
				SpawnPointDef{
					.id = SpawnPointIds::PlazaPlayerStart,
					.name = "Plaza.PlayerStart",
					.position = WorldVec3Def{ 480.167800f, 5.508454f, 481.655600f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				//SpawnPointDef{
				//	.id = SpawnPointIds::PlazaMonster01,
				//	.name = "Plaza.DemonExecutioner.01",
				//	.position = WorldVec3Def{ 460.167800f, 5.508454f, 481.655600f },
				//	.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				//},
			},
			// TODO(content): replace temporary navmesh path with Plaza map data.
			.navMesh = MapNavMeshDef
			{
				.navMeshBinPath = "../Map/Plaza_NavMesh_v1.bin",
				.agentRadius = 0.35f,
				.agentHeight = 2.0f,
				.agentMaxClimb = 0.4f,
				.agentMaxSlope = 45.0f
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
			.initialSpawnSetId = SpawnSetId::PlazaDefault,
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
					.position = WorldVec3Def{ 161.352478f, 48.737797f, 644.831543f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::VillageMonster01,
					.name = "Village.Imp.01",
					.position = WorldVec3Def{ 212.904114f, 56.205055f, 620.986938f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::VillageMonster02,
					.name = "Village.Imp.02",
					.position = WorldVec3Def{ 217.488998f, 56.171982f, 579.312317f },
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
			},
			// TODO(content): fill Village navmesh, navigation profile, and environment tags.
			.navMesh = MapNavMeshDef
			{
				.navMeshBinPath = "../Map/Village_NavMesh_v10.bin",
				.agentRadius = 0.35f,
				.agentHeight = 2.0f,
				.agentMaxClimb = 0.4f,
				.agentMaxSlope = 45.0f
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
					.position = WorldVec3Def{ 327.722809, 67.219727, 226.415680 },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::CastleMonster01,
					.name = "Castle.Imp.01",
					.position = WorldVec3Def{ 326.687042f, 69.436005f, 326.435638f },
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
			},
			// TODO(content): fill Castle navmesh, navigation profile, and environment tags.
			.navMesh = MapNavMeshDef
			{
				.navMeshBinPath = "../Map/Castle_NavMesh_v2.bin",
				.agentRadius = 0.35f,
				.agentHeight = 2.0f,
				.agentMaxClimb = 0.4f,
				.agentMaxSlope = 45.0f
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
					.name = "Final.Imp.01",
					.position = WorldVec3Def{ 482.0f, 48.737797f, 482.0f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
			},
			// TODO(content): fill Final navmesh, navigation profile, and environment tags.
			.navMesh = MapNavMeshDef
			{
				.navMeshBinPath = "../Map/Cathedral_NavMesh_v2.bin",
				.agentRadius = 0.35f,
				.agentHeight = 2.0f,
				.agentMaxClimb = 0.4f,
				.agentMaxSlope = 45.0f
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

		// TODO(content): add scripted Plaza/Pvp choice link metadata after Final clear UI is defined.
		.linkRules = {},
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
			.resourceId = 5,
			.defaultPlayerSpawnPointId = SpawnPointIds::PvpPlayerStartA,
			.spawnPoints = {
				SpawnPointDef{
					.id = SpawnPointIds::PvpPlayerStartA,
					.name = "Pvp.PlayerStart.A",
					.position = WorldVec3Def{ -5.0f, 5.0f, 0.0f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
				SpawnPointDef{
					.id = SpawnPointIds::PvpPlayerStartB,
					.name = "Pvp.PlayerStart.B",
					.position = WorldVec3Def{ 5.0f, 5.0f, 0.0f },
					.rotation = WorldQuatDef{ 0.0f, 0.0f, 0.0f, 1.0f },
				},
			},
			// TODO(content): add arena-specific navmesh and team spawn groups.
			.navMesh = std::nullopt,
			.navigationProfile = std::nullopt,
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

		// TODO(content): replace Field kind with Arena when PvP rule data is introduced.
		.linkRules = {},
		.executionModelKey = executionModelKey,
		.transferProfileId = PlayerCharacterWorldTransferProfileId,
	};
}
