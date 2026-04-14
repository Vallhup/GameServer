#include "pch.h"
#include "ImpCombatActionPolicy.h"

#include "IAIState.h"
#include "System.h"
#include "ECS/GameplayRuntimeComponents.h"
#include "TransformHelper.h"

#include <cmath>

static constexpr float kDirectionEpsilon = 1.0e-4f;

static int PseudoRand(uint64_t entityId, uint32_t sequence, int range)
{
    uint64_t x =
        entityId ^
        (static_cast<uint64_t>(sequence) + 0x9e3779b97f4a7c15ull);
    x ^= (x >> 30);
    x *= 0xbf58476d1ce4e5b9ull;
    x ^= (x >> 27);
    x *= 0x94d049bb133111ebull;
    x ^= (x >> 31);
    return static_cast<int>(x % static_cast<uint64_t>(range));
}

static ActionId PickImpMeleeAction(ActionId lastUsed, int roll)
{
    switch (lastUsed) {
    case ActionId::Imp_melee1:
        if (roll < 35)
            return ActionId::Imp_melee3;
        if (roll < 70)
            return ActionId::Imp_melee4;
        return ActionId::Imp_melee5;

    case ActionId::Imp_melee3:
        if (roll < 10)
            return ActionId::Imp_melee1;
        if (roll < 55)
            return ActionId::Imp_melee4;
        return ActionId::Imp_melee5;

    case ActionId::Imp_melee4:
        if (roll < 10)
            return ActionId::Imp_melee1;
        if (roll < 50)
            return ActionId::Imp_melee3;
        return ActionId::Imp_melee5;

    case ActionId::Imp_melee5:
        if (roll < 10)
            return ActionId::Imp_melee1;
        if (roll < 55)
            return ActionId::Imp_melee3;
        return ActionId::Imp_melee4;

    default:
        if (roll < 10)
            return ActionId::Imp_melee1;
        if (roll < 45)
            return ActionId::Imp_melee3;
        if (roll < 75)
            return ActionId::Imp_melee4;
        return ActionId::Imp_melee5;
    }
}

static void FillAttackDirectionTowardTarget(const AIContext& ctx, CombatActionSelection& out)
{
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

CombatActionSelection ImpCombatActionPolicy::SelectAction(const AIContext& ctx) const
{
	CombatActionSelection result{};

	if (ctx.decision->attackCooldownAcc < ctx.decisionTuning->attackCooldown)
		return result;

    result.shouldAttack = true;
    FillAttackDirectionTowardTarget(ctx, result);

	const ActionId lastUsed = ctx.blackboard->lastUsedActionId;
	const int roll = PseudoRand(ctx.self.id, ctx.blackboard->combatActionSequence, 100);

	result.selectedActionId = PickImpMeleeAction(lastUsed, roll);
	return result;
}
