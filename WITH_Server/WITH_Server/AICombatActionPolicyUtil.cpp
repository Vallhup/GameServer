#include "pch.h"
#include "AICombatActionPolicyUtil.h"

#include "IAIState.h"
#include "System.h"
#include "TransformHelper.h"

#include <algorithm>
#include <cmath>

namespace
{
	constexpr float kDirectionEpsilon = 1.0e-4f;
}

int AICombatActionPolicyUtil::PseudoRand(
	uint64_t entityId,
	uint32_t sequence,
	int range)
{
	if (range <= 0)
		return 0;

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

void AICombatActionPolicyUtil::FillAttackDirectionTowardTarget(
	const AIContext& ctx,
	CombatActionSelection& out)
{
	if (ctx.perception != nullptr &&
		ctx.sysCtx != nullptr &&
		ctx.selfTr != nullptr &&
		!ctx.perception->selectedTarget.IsNull())
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

	if (ctx.selfTr == nullptr)
		return;

	const XMVECTOR forward = TransformHelper::Forward(*ctx.selfTr);
	XMFLOAT3 forwardF{};
	XMStoreFloat3(&forwardF, forward);

	const float length =
		std::sqrt(forwardF.x * forwardF.x + forwardF.z * forwardF.z);
	if (length > kDirectionEpsilon)
	{
		out.directionX = forwardF.x / length;
		out.directionZ = forwardF.z / length;
	}
}

ActionId AICombatActionPolicyUtil::PickWeightedAction(
	std::span<const WeightedActionEntry> actions,
	ActionId lastUsed,
	int roll,
	uint16_t repeatWeightPercent)
{
	uint32_t totalWeight = 0;
	for (const WeightedActionEntry& entry : actions)
	{
		uint32_t weight = entry.weight;
		if (entry.actionId == lastUsed && actions.size() > 1)
		{
			weight =
				(weight * static_cast<uint32_t>(repeatWeightPercent)) / 100u;
		}
		totalWeight += weight;
	}

	if (totalWeight == 0)
		return ActionId::None;

	uint32_t cursor =
		static_cast<uint32_t>(std::max(0, roll)) % totalWeight;
	for (const WeightedActionEntry& entry : actions)
	{
		uint32_t weight = entry.weight;
		if (entry.actionId == lastUsed && actions.size() > 1)
		{
			weight =
				(weight * static_cast<uint32_t>(repeatWeightPercent)) / 100u;
		}

		if (weight == 0)
			continue;

		if (cursor < weight)
			return entry.actionId;

		cursor -= weight;
	}

	return ActionId::None;
}
