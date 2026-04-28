#pragma once

#include "ICharacterAspect.h"

class AIControlAspect final : public ICharacterAspect {
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
