#pragma once

#include "DefLoadResult.h"

struct AIActionDef;
struct AIMovementProfileDef;
struct AIReactionRuleDef;
struct AIPhaseTransitionDef;
class GameDataCatalog;

class GamePlayDefValidator final {
public:
	static DefLoadResult ValidateGameplayDefs(const GameDataCatalog& catalog);

private:
	static bool ValidateAIAction(
		const AIActionDef& action,
		const char* owner,
		std::string& outError);
	static bool ValidateMovementProfile(
		const AIMovementProfileDef& profile,
		std::string& outError);
	static bool ValidateReactionRule(
		const AIReactionRuleDef& rule,
		std::string& outError);
	static bool ValidatePhaseTransition(
		const AIPhaseTransitionDef& transition,
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
