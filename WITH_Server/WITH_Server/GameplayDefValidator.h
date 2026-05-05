#pragma once

#include "DefLoadResult.h"

struct WeightedActionEntry;
class GameDataCatalog;

class GamePlayDefValidator final {
public:
	static DefLoadResult ValidateGameplayDefs(const GameDataCatalog& catalog);

private:
	static bool ValidateWeightedAction(
		const WeightedActionEntry& entry,
		const char* owner,
		const GameDataCatalog& catalog,
		std::string& outError);

	static bool ValidateAIBehaviorProfiles(
		const GameDataCatalog& catalog,
		std::string& outError);
	static bool ValidateCharacterDefs(
		const GameDataCatalog& catalog,
		std::string& outError);
	static bool ValidateSpawnSetDefs(
		const GameDataCatalog& catalog,
		std::string& outError);
};
