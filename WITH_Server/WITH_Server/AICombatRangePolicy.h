#pragma once

struct AIContext;

class AICombatRangePolicy final {
public:
	static bool ShouldEnterCombat(const AIContext& ctx);
	static bool ShouldExitCombat(const AIContext& ctx) noexcept;
	static bool ShouldHoldChasePosition(const AIContext& ctx);
	static bool CanSelectCombatAction(const AIContext& ctx);

private:
	static double ResolveCombatEnterRange(const AIContext& ctx) noexcept;
	static double ResolveCombatExitRange(const AIContext& ctx) noexcept;
};
