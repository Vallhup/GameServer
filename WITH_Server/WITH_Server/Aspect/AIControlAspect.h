#pragma once

#include "ICharacterAspect.h"
#include "../ECS/Components/GameplayAIComponents.h"

struct AIBehaviorProfileDef;

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

private:
	static AIBlackboardComp BuildAIBlackboardComp(
		const AssembleParams& params,
		const AIBehaviorProfileDef* profile) noexcept;

	static AIActionRuntimeComp BuildAIActionRuntimeComp(
		const AIBehaviorProfileDef* profile) noexcept;
};
