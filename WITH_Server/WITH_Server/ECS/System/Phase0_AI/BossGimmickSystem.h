#pragma once

#include "../../GameplayRuntimeComponents.h"
#include "System.h"
#include "SystemMetaStorage.h"

class BossGimmickSystem final : public System {
	static const StaticSystemMetaStorage<18> kMetaStorage;
	static constexpr float kLockRefreshSec{ 0.25f };

public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override { return kMetaStorage.meta; }

private:
	void TickPhaseTransitionObjects(
		SystemContext& ctx,
		Entity boss,
		BossGimmickStateComp& gimmick,
		float dtSec);

	void TickFinalSafeZone(
		SystemContext& ctx,
		Entity boss,
		BossGimmickStateComp& gimmick,
		float dtSec);
};
