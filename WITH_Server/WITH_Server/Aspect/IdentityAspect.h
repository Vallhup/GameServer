#pragma once

#include "ICharacterAspect.h"

// 모든 캐릭터의 공통 식별 + 위치. SpawnTypeComp 와 WorldTransformComp.
class IdentityAspect final : public ICharacterAspect {
public:
	CharacterFeatureFlags RequiredFeature() const noexcept override;
	void RegisterStorages(WorldRuntime& runtime) const override;
	void Attach(
		WorldRuntime& runtime,
		Entity entity,
		const CharacterDef& def,
		const AssembleParams& params) const override;
};
