#include "pch.h"
#include "WorldDef.h"

WorldDef CreatePlazaWorldDef(WorldExecutionModelKey executionModelKey)
{
	WorldDef def{};
	def.id = WorldDefId::Plaza;
	def.name = "Plaza";

	def.topology.kind = WorldKind::Hub;
	def.topology.instanceType = WorldInstanceType::Persistent;

	// 프로그램 시작과 함께 생성되는 기본 허브 월드.
	// 게임이 동작하는 동안 유지되는 비전투용 마을을 전제로 한다.
	def.entryPolicy.creationPolicy = CreationPolicy::PreCreated;
	def.entryPolicy.joinPolicy = JoinPolicy::FreeJoin;
	def.entryPolicy.maxPlayerCount = 5000;
	def.entryPolicy.allowReEntry = true;
	def.entryPolicy.destroyWhenEmpty = false;
	def.entryPolicy.emptyDestroyDelaySec = std::nullopt;
	def.entryPolicy.fallbackWorldDefId = std::nullopt;
	// TODO: Plaza를 기본 fallback 도착지로 삼을 게임 월드들은
	//       각자의 entryPolicy.fallbackWorldDefId = WorldDefId::Plaza 로 연결한다.

	// TODO: 실제 마을 맵 콘텐츠 ID와 스폰 포인트 ID가 확정되면 교체한다.
	def.map.resourceId = 0;
	def.map.defaultPlayerSpawnPointId = 0;
	def.map.namedSpawnPoints.clear();
	def.map.navigationProfileId = std::nullopt;
	def.map.environmentTags.clear();
	// TODO: 안전 구역, 상점 구역, 포탈 허브 등 환경 태그 체계를 붙인다.

	// TODO: 실제 Plaza NavMesh export 결과에 맞춰 경로와 agent 파라미터를 검증한다.
	def.map.navMesh = MapNavMeshDef
	{
		.navMeshBinPath = "../Map/Village_NavMesh_v3.bin",
		.agentRadius = 0.35f,
		.agentHeight = 2.0f,
		.agentMaxClimb = 0.4f,
		.agentMaxSlope = 45.0f,
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
	// TODO: 안전 지대 전용 이동 제약이나 NPC/플레이어 분리 필터가 필요하면 프로필을 세분화한다.

	def.spawn.initialSpawnSetId = SpawnSetId::PlazaDefault;
	def.spawn.respawnSpawnSetId = std::nullopt;
	// TODO: PlazaDefault 스폰셋과 귀환/재접속 위치 규칙을 실제 콘텐츠와 맞춘다.

	def.progressRule.clearType = WorldClearConditionType::None;
	def.progressRule.failType = WorldFailConditionType::None;
	def.progressRule.completionType = WorldCompletionActionType::None;
	def.progressRule.completionDelaySec = std::nullopt;
	def.progressRule.autoCloseOnComplete = false;
	// TODO: 허브 월드 공용 이벤트 규칙이 생기면 progressRule 또는 별도 정책으로 분리한다.

	def.linkRules.clear();
	def.executionModelKey = executionModelKey;
	def.transferProfileId = InvalidWorldTransferProfileId;
	return def;
}
