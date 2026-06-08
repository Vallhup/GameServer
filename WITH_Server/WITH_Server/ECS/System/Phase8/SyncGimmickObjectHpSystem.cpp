#include "pch.h"
#include "SyncGimmickObjectHpSystem.h"

#include "../GameplaySystemUtil.h"

using namespace GameplaySystemUtil;

namespace
{
	uint32_t ClampHp(int32_t hp) noexcept
	{
		return static_cast<uint32_t>(std::max(0, hp));
	}

	uint64_t ToGimmickNetId(Entity entity) noexcept
	{
		return entity.IsNull() ? 0ull : entity.id;
	}

	bool EmitObjectUpdate(
		SystemContext& ctx,
		Entity object,
		const GimmickObjectComp& gimmick,
		const WorldTransformComp& transform,
		const StaticBoxHurtColliderComp& box,
		const CombatStatStateComp& stats)
	{
		PendingBossGimmickReplicationComp* pending =
			ctx.ecs.GetMutableComponent<PendingBossGimmickReplicationComp>(
				gimmick.ownerBoss);
		if (pending == nullptr)
		{
			return false;
		}

		const BossGimmickStateComp* bossGimmick =
			ctx.ecs.GetComponent<BossGimmickStateComp>(gimmick.ownerBoss);

		pending->objectEvents.push_back(
			PendingBossGimmickObjectSyncEvent{
				.boss = gimmick.ownerBoss,
				.gimmickSeq =
					bossGimmick != nullptr ? bossGimmick->gimmickSeq : 0,
				.objectNetId = ToGimmickNetId(object),
				.state = BossGimmickObjectSyncState::Updated,
				.position = transform.position,
				.radius = std::max(box.halfExtents.x, box.halfExtents.z),
				.curHp = ClampHp(stats.currentHp),
				.maxHp = ClampHp(stats.maxHp)
			});
		return true;
	}
}

const StaticSystemMetaStorage<8> SyncGimmickObjectHpSystem::kMetaStorage =
	MakeMetaStorage(
		SysTag<SyncGimmickObjectHpSystem>(),
		"SyncGimmickObjectHpSystem",
		std::array<AccessSpec, 8>
	{
		WriteImmediate(ComponentRes<GimmickObjectComp>()),
		ReadImmediate(ComponentRes<CombatStatStateComp>()),
		ReadImmediate(ComponentRes<WorldTransformComp>()),
		ReadImmediate(ComponentRes<StaticBoxHurtColliderComp>()),
		ReadImmediate(ComponentRes<BossGimmickStateComp>()),
		WriteImmediate(ComponentRes<PendingBossGimmickReplicationComp>()),
		ReadImmediate(ComponentRes<PendingDespawnTag>()),
		ReadImmediate(ComponentRes<PendingWorldTransferTag>()),
	});

void SyncGimmickObjectHpSystem::Execute(SystemContext& ctx)
{
	for (auto [entity, gimmick, stats, transform, box] :
		ctx.ecs.View<
			GimmickObjectComp,
			CombatStatStateComp,
			WorldTransformComp,
			StaticBoxHurtColliderComp>())
	{
		if (gimmick.broken ||
			gimmick.lastSyncedHp == stats.currentHp ||
			HasBlockingPendingState(ctx.ecs, entity))
		{
			continue;
		}

		if (EmitObjectUpdate(ctx, entity, gimmick, transform, box, stats))
		{
			gimmick.lastSyncedHp = stats.currentHp;
		}
	}
}
