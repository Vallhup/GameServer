#pragma once

#include "ICharacterAspect.h"
#include <ECS/Components/GameplayCombatComponents.h>

// 액션/로코모션/애니메이션/콜라이더/스탯/버프/펜딩프레젠테이션.
// 전투 가능한 모든 캐릭터의 공통 베이스.
class BaseCombatantAspect final : public ICharacterAspect {
public:
	CharacterFeatureFlags RequiredFeature() const noexcept override;

	void RegisterStorages(WorldRuntime& runtime) const override;

	void Attach(
		WorldRuntime& runtime,
		Entity entity,
		const CharacterDef& def,
		const AssembleParams& params) const override;

	bool Validate(
		const CharacterDef& def,
		std::string& outError) const override;

private:
	static CombatStatStateComp BuildCombatStatState(
		const CharacterDef& def,
		const std::optional<CombatStatInitialState>& overrideStats) noexcept;
};
