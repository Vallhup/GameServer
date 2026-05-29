#include "pch.h"
#include "RecordCombatStatisticsSystem.h"

#include "../../../CharacterDef.h"
#include "../../../CombatStatisticsCommands.h"
#include "../../../CombatStatisticsTaskTypeIds.h"
#include "../../../ODBCDatabaseBackend.h"
#include "../../../PacketHandlerContext.h"
#include "../../../SessionFlowController.h"
#include "DBData.h"
#include "ECS/Components/GameplayInputComponents.h"
#include "ECS/Components/GameplayWorldLifecycleComponents.h"
#include "FrameworkLog.h"
#include "SystemMetaHelper.h"
#include "SystemMetaStorage.h"

namespace
{
    constexpr const char* kLogCategory = "RecordCombatStatistics";
}

// ---------------------------------------------------------------------------
// AccessSpec 선언 (3개 — StaticSystemMetaStorage<3> 와 일치해야 한다):
//   [0] WriteImmediate(PendingMonsterKillEventComp)   — 킬 이벤트 소비
//   [1] WriteImmediate(PendingPlayerDeathCountEventComp) — 사망 이벤트 소비
//   [2] ReadImmediate(PlayerControlIdentityComp)      — 세션 ID 조회
// ---------------------------------------------------------------------------
const StaticSystemMetaStorage<3> RecordCombatStatisticsSystem::kMetaStorage =
    MakeMetaStorage(
        SysTag<RecordCombatStatisticsSystem>(),
        "RecordCombatStatisticsSystem",
        std::array<AccessSpec, 3>
        {
            WriteImmediate(ComponentRes<PendingMonsterKillEventComp>()),
            WriteImmediate(ComponentRes<PendingPlayerDeathCountEventComp>()),
            ReadImmediate(ComponentRes<PlayerControlIdentityComp>()),
        });

void RecordCombatStatisticsSystem::Execute(SystemContext& ctx)
{
    auto& svc = PacketHandlerContext::Get();
    if (svc.database == nullptr || svc.sessionFlow == nullptr)
        return;

    if (g_incrementMonsterKillCountResultTaskTypeId    == InvalidDynamicTaskTypeId ||
        g_incrementDeathByMonsterCountResultTaskTypeId == InvalidDynamicTaskTypeId)
        return;

    // -----------------------------------------------------------------------
    // [1] 몬스터 처치 이벤트 소비
    //
    // CommitCombatResultSystem 이 같은 프레임에서 킬러 엔티티에
    // PendingMonsterKillEventComp.pending = true 를 세팅한다.
    // 이 블록에서 이벤트를 소비하고 DB 커맨드를 제출한 뒤 전체 초기화한다.
    // -----------------------------------------------------------------------
    for (auto [entity, killEvent] :
        ctx.ecs.View<PendingMonsterKillEventComp>())
    {
        if (!killEvent.pending)
            continue;

        const PlayerControlIdentityComp* identity =
            ctx.ecs.GetComponent<PlayerControlIdentityComp>(entity);
        if (identity == nullptr)
        {
            // 플레이어가 아닌 엔티티에 이벤트가 세팅된 경우: 방어적 초기화.
            killEvent = PendingMonsterKillEventComp{};
            continue;
        }

        const SessionId ownerSessionId = identity->ownerSessionId;
        const SessionFlow* flow = svc.sessionFlow->FindFlow(ownerSessionId);
        if (flow == nullptr || flow->accountId == 0)
        {
            killEvent = PendingMonsterKillEventComp{};
            FWLOG_WARN(kLogCategory,
                "MonsterKill: session flow not found, skipping DB command (sid=%u)",
                ownerSessionId);
            continue;
        }

        DBCommandEnvelope envelope{};
        envelope.meta.sessionId            = ownerSessionId;
        envelope.meta.scopeId              = ctx.execScopeId;
        envelope.meta.requestFrameIndex    = 0;
        envelope.meta.completionTaskTypeId =
            g_incrementMonsterKillCountResultTaskTypeId;
        envelope.command = std::make_unique<IncrementMonsterKillCountCommand>(
            flow->accountId,
            static_cast<uint8_t>(killEvent.killedCharacterId),
            ownerSessionId);

        const DBRequestId requestId = svc.database->Submit(std::move(envelope));
        if (requestId == InvalidDBRequestId)
        {
            FWLOG_WARN(kLogCategory,
                "MonsterKill: DB submit rejected (sid=%u, accountId=%llu)",
                ownerSessionId,
                flow->accountId);
        }

        killEvent = PendingMonsterKillEventComp{};
    }

    // -----------------------------------------------------------------------
    // [2] 몬스터에 의한 사망 이벤트 소비
    //
    // CommitCombatResultSystem / ResolveDeathAndDespawnSystem 이 피해자 엔티티에
    // PendingPlayerDeathCountEventComp.pending = true 와 killerCharacterId 를 세팅한다.
    // killerCharacterId == CharacterId{} 이면 비-몬스터 사망이므로 건너뛴다.
    // -----------------------------------------------------------------------
    for (auto [entity, deathEvent] :
        ctx.ecs.View<PendingPlayerDeathCountEventComp>())
    {
        if (!deathEvent.pending)
            continue;

        // killerCharacterId 가 세팅되지 않은 경우: 비-몬스터 사망 또는 이미 소비됨.
        if (deathEvent.killerCharacterId == CharacterId{})
        {
            deathEvent = PendingPlayerDeathCountEventComp{};
            continue;
        }

        const PlayerControlIdentityComp* identity =
            ctx.ecs.GetComponent<PlayerControlIdentityComp>(entity);
        if (identity == nullptr)
        {
            deathEvent = PendingPlayerDeathCountEventComp{};
            continue;
        }

        const SessionId ownerSessionId = identity->ownerSessionId;
        const SessionFlow* flow = svc.sessionFlow->FindFlow(ownerSessionId);
        if (flow == nullptr || flow->accountId == 0)
        {
            deathEvent = PendingPlayerDeathCountEventComp{};
            FWLOG_WARN(kLogCategory,
                "DeathByMonster: session flow not found, skipping DB command (sid=%u)",
                ownerSessionId);
            continue;
        }

        DBCommandEnvelope envelope{};
        envelope.meta.sessionId            = ownerSessionId;
        envelope.meta.scopeId              = ctx.execScopeId;
        envelope.meta.requestFrameIndex    = 0;
        envelope.meta.completionTaskTypeId =
            g_incrementDeathByMonsterCountResultTaskTypeId;
        envelope.command = std::make_unique<IncrementDeathByMonsterCountCommand>(
            flow->accountId,
            static_cast<uint8_t>(deathEvent.killerCharacterId),
            ownerSessionId);

        const DBRequestId requestId = svc.database->Submit(std::move(envelope));
        if (requestId == InvalidDBRequestId)
        {
            FWLOG_WARN(kLogCategory,
                "DeathByMonster: DB submit rejected (sid=%u, accountId=%llu)",
                ownerSessionId,
                flow->accountId);
        }

        deathEvent = PendingPlayerDeathCountEventComp{};
    }
}
