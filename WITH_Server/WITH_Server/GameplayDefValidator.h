#pragma once

#include "DefLoadResult.h"

struct WeightedActionEntry;

class GamePlayDefValidator final {
public:
	static DefLoadResult ValidateGameplayDefs();

private:
	static bool ValidateWeightedAction(
		const WeightedActionEntry& entry,
		const char* owner,
		std::string& outError);

	static bool ValidateAIBehaviorProfiles(std::string& outError);
	static bool ValidateActionDefs(std::string& outError);
	static bool ValidateCharacterDefs(std::string& outError);
	static bool ValidateBuffDefs(std::string& outError);
	static bool ValidateSpawnSetDefs(std::string& outError);
};

