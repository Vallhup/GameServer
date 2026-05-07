#pragma once

#include <DirectXMath.h>

struct AIContext;

namespace AIMovementPolicyUtil
{
	void ClearMove(AIContext& ctx);

	bool StoreMoveDirection(
		AIContext& ctx,
		DirectX::XMFLOAT3 moveDir,
		bool wantsRun);

	bool TryGetCurrentTargetPosition(
		AIContext& ctx,
		DirectX::XMFLOAT3& outTargetPos);

	void BuildDestinationIntent(
		AIContext& ctx,
		const DirectX::XMFLOAT3& destination,
		bool wantsRun);
}
