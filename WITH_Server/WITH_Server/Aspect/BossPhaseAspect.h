#pragma once

#include "ICharacterAspect.h"

// 보스 전용 컴포넌트는 아직 정의되지 않았다. placeholder.
class BossPhaseAspect final : public ICharacterAspect {
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
};
