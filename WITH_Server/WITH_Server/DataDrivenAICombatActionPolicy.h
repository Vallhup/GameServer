#pragma once

#include "IAICombatActionPolicy.h"

class DataDrivenAICombatActionPolicy final : public IAICombatActionPolicy {
public:
	virtual ~DataDrivenAICombatActionPolicy() = default;

	virtual CombatActionSelection SelectAction(
		const AIContext& ctx) const override;

	virtual void CommitSelection(
		AIContext& ctx,
		const CombatActionSelection& selection) const override;
};
