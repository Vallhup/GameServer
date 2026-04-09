#pragma once

#include "System.h"
#include "../../GameplayRuntimeComponents.h"

class AdvanceActionTimelineSystem final : public System {
public:
	void Execute(SystemContext& ctx) override;
	const SystemMeta& Meta() const override;

private:
	static const SystemMeta kMeta;

	static void ApplyElapsedToActiveAction(
		ActionStateComp& actionState,
		const ActionTimelineAdvanceComp& advance);

	static void CollectTimelineEvents(
		const ActionDef& actionDef,
		ActionTimelineAdvanceComp& advance);
};
