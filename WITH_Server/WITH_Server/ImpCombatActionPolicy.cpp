#include "pch.h"
#include "ImpCombatActionPolicy.h"

#include "IAIState.h"
#include "System.h"
#include "ECS/GameplayRuntimeComponents.h"
#include "TransformHelper.h"

#include <cmath>

// ── 내부 헬퍼 ────────────────────────────────────────────────────────────────

static constexpr float kDirectionEpsilon = 1.0e-4f;

// 엔티티 ID 를 시드로 사용하는 결정적 의사난수 (per-call, stateless)
// 완전한 랜덤이 아니라 Entity 별로 다른 패턴을 내도록 분산만 보장한다.
int PseudoRand(uint64_t entityId, int range)
{
    // xorshift64 로 빠르게 분산
    uint64_t x = entityId ^ (entityId << 13);
    x ^= (x >> 7);
    x ^= (x << 17);
    return static_cast<int>(x % static_cast<uint64_t>(range));
}

// 공격 방향: 자신 → 타겟 방향 벡터 (XZ 평면)
// 타겟을 찾지 못하거나 거리가 너무 가까우면 폴백으로 자신의 forward 를 사용한다.
static void FillAttackDirectionTowardTarget(const AIContext& ctx, CombatActionSelection& out)
{
    // 타겟 위치 조회
    if (ctx.perception != nullptr && !ctx.perception->selectedTarget.IsNull())
    {
        const WorldTransformComp* targetTr =
            ctx.sysCtx->ecs.GetComponent<WorldTransformComp>(
                ctx.perception->selectedTarget);

        if (targetTr != nullptr)
        {
            const float dx = targetTr->position.x - ctx.selfTr->position.x;
            const float dz = targetTr->position.z - ctx.selfTr->position.z;
            const float length = std::sqrt(dx * dx + dz * dz);

            if (length > kDirectionEpsilon)
            {
                out.directionX = dx / length;
                out.directionZ = dz / length;
                return;
            }
        }
    }

    // 폴백: 자신의 forward (-Z 로컬 축을 월드 공간으로 회전)
    // TransformHelper::Forward 가 동일한 변환을 수행하므로 재사용한다.
    const XMVECTOR forward = TransformHelper::Forward(*ctx.selfTr);
    XMFLOAT3 forwardF{};
    XMStoreFloat3(&forwardF, forward);

    const float length = std::sqrt(forwardF.x * forwardF.x + forwardF.z * forwardF.z);
    if (length > kDirectionEpsilon)
    {
        out.directionX = forwardF.x / length;
        out.directionZ = forwardF.z / length;
    }
}

// ── ImpCombatActionPolicy::SelectAction ──────────────────────────────────────

CombatActionSelection ImpCombatActionPolicy::SelectAction(const AIContext& ctx) const
{
	CombatActionSelection result{};

	// 쿨다운 미충족 시 공격하지 않음
	if (ctx.decision->attackCooldownAcc < ctx.decisionTuning->attackCooldown)
		return result;

    result.shouldAttack = true;
    FillAttackDirectionTowardTarget(ctx, result);

	const ActionId lastUsed = ctx.blackboard->lastUsedActionId;

	// 직전 공격이 melee1 이면 30% 확률로 melee2 연계
	// 단조로운 melee1 반복을 방지하는 최소한의 다양성 보장
	if (lastUsed == ActionId::Imp_melee1)
	{
		const int roll = PseudoRand(ctx.self.id, 10);
		result.selectedActionId = (roll < 3)
			? ActionId::Imp_melee3
			: ActionId::Imp_melee1;
		return result;
	}

	// 그 외 기본: melee1
	result.selectedActionId = ActionId::Imp_melee1;
	return result;
}
                             