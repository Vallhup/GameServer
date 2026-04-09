#pragma once

#include "ICharacterAspect.h"

class ReplicationAspect final : public ICharacterAspect {
public:
	CharacterFeatureFlags RequiredFeature() const noexcept override;
	void RegisterStorages(WorldRuntime& runtime) const override;
	void Attach(
		WorldRuntime& runtime,
		Entity entity,
		const CharacterDef& def,
		const AssembleParams& params) const override;
};
