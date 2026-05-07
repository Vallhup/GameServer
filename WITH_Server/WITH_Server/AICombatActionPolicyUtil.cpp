#include "pch.h"
#include "AICombatActionPolicyUtil.h"

#include "IAICombatActionPolicy.h"
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
