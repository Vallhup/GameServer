#pragma once

#include <cstdint>

using AbilityId = uint16_t;
using AbilitySetId = uint16_t;
using AbilityInputBindingProfileId = uint16_t;
using AbilityAnimationBindingProfileId = uint16_t;
using AbilityFallbackReactionProfileId = uint16_t;
using GameplayEffectId = uint16_t;
using GameplayTagId = uint16_t;
using AttributeId = uint16_t;

inline constexpr AbilityId InvalidAbilityId = 0;
inline constexpr AbilitySetId InvalidAbilitySetId = 0;
inline constexpr AbilityInputBindingProfileId InvalidAbilityInputBindingProfileId = 0;
inline constexpr AbilityAnimationBindingProfileId InvalidAbilityAnimationBindingProfileId = 0;
inline constexpr AbilityFallbackReactionProfileId InvalidAbilityFallbackReactionProfileId = 0;
inline constexpr GameplayEffectId InvalidGameplayEffectId = 0;
inline constexpr GameplayTagId InvalidGameplayTagId = 0;
inline constexpr AttributeId InvalidAttributeId = 0;

using GameplayTagMask = uint64_t;
