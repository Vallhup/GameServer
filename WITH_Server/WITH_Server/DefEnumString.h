#pragma once

#include <string_view>

enum class AIArchetype : uint8_t;
enum class BodyPushability : uint8_t;
enum class CharacterFeatureFlags : uint32_t;
enum class CharacterId : uint8_t;
enum class CharacterRole : uint8_t;
enum class Faction : uint8_t;
enum class LocomotionMode : uint8_t;
enum class RespawnPolicy : uint8_t;
enum class SpawnConditionType : uint8_t;
enum class AnimationId : uint8_t;
enum class SpawnSetId : uint8_t;
enum class AIActionDistanceBucket : uint8_t;
enum class AIActionRole : uint8_t;
enum class AIMovementBehavior : uint8_t;
enum class AIReactionRuleEvent : uint8_t;
enum class AIReactionRuleOutcome : uint8_t;
enum class TitleConditionType : uint8_t;

using SpawnPointId = uint16_t;

bool ParseDefString(std::string_view text, AIArchetype& outValue) noexcept;
bool ParseDefString(std::string_view text, BodyPushability& outValue) noexcept;
bool ParseDefString(std::string_view text, CharacterFeatureFlags& outValue) noexcept;
bool ParseDefString(std::string_view text, CharacterId& outValue) noexcept;
bool ParseDefString(std::string_view text, CharacterRole& outValue) noexcept;
bool ParseDefString(std::string_view text, Faction& outValue) noexcept;
bool ParseDefString(std::string_view text, LocomotionMode& outValue) noexcept;
bool ParseDefString(std::string_view text, RespawnPolicy& outValue) noexcept;
bool ParseDefString(std::string_view text, SpawnConditionType& outValue) noexcept;
bool ParseDefString(std::string_view text, SpawnSetId& outValue) noexcept;
bool ParseDefString(std::string_view text, AIActionDistanceBucket& outValue) noexcept;
bool ParseDefString(std::string_view text, AIActionRole& outValue) noexcept;
bool ParseDefString(std::string_view text, AIMovementBehavior& outValue) noexcept;
bool ParseDefString(std::string_view text, AIReactionRuleEvent& outValue) noexcept;
bool ParseDefString(std::string_view text, AIReactionRuleOutcome& outValue) noexcept;
bool ParseDefString(std::string_view text, TitleConditionType& outValue) noexcept;

bool ParseAnimationIdString(std::string_view text, AnimationId& outValue) noexcept;
bool ParseSpawnPointIdString(std::string_view text, SpawnPointId& outValue) noexcept;
