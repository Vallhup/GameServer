#include "pch.h"
#include "AbilitySetDef.h"

#include "DefCompilePipeline.h"
#include "DefEnumString.h"
#include "DefJsonFileLoader.h"
#include "DefJsonReader.h"
#include "DefKeyIndex.h"
#include "ECS/Components/GameplayAbilityComponents.h"

#include <algorithm>
#include <iterator>

using json = nlohmann::json;

namespace
{
	struct AbilityGrantDto
	{
		std::string abilityKey;
		uint16_t level{ 1 };
	};

	struct AbilityInputBindingEntryDto
	{
		AbilityRequestSemantic request{ AbilityRequestSemantic::None };
		std::vector<std::string> candidateAbilityKeys;
		AbilityCandidateSelectionPolicy selectionPolicy{
			AbilityCandidateSelectionPolicy::OrderedFirstValid };
		int priority{ 0 };
	};

	struct AbilityInputBindingProfileDto
	{
		std::string key;
		std::vector<AbilityInputBindingEntryDto> entries;
	};

	struct AbilityAnimationBindingDto
	{
		std::string abilityKey;
		std::string animationKey;
		float playRate{ 1.0f };
		float startNormalizedTime{ 0.0f };
	};

	struct AbilityLocomotionAnimationBindingDto
	{
		LocomotionMode mode{ LocomotionMode::Idle };
		std::string animationKey;
		float playRate{ 1.0f };
		bool holdLastFrame{ false };
	};

	struct AbilityAnimationBindingProfileDto
	{
		std::string key;
		std::vector<AbilityAnimationBindingDto> abilityBindings;
		std::vector<AbilityLocomotionAnimationBindingDto> locomotionBindings;
	};

	struct AbilityFallbackReactionEntryDto
	{
		AbilityTransitionCause cause{ AbilityTransitionCause::OnHitReceived };
		std::string toAbilityKey;
		int priority{ 0 };
	};

	struct AbilityFallbackReactionProfileDto
	{
		std::string key;
		std::vector<AbilityFallbackReactionEntryDto> entries;
	};

	struct AbilitySetDto
	{
		std::string key;
		std::string name;
		std::vector<AbilityGrantDto> grants;
		std::optional<std::string> inputBindingProfileKey;
		std::optional<std::string> animationBindingProfileKey;
		std::optional<std::string> fallbackReactionProfileKey;
	};

	struct ParsedAbilitySetDefinitions
	{
		std::vector<AbilitySetDto> abilitySets;
		std::vector<AbilityInputBindingProfileDto> inputBindingProfiles;
		std::vector<AbilityAnimationBindingProfileDto> animationBindingProfiles;
		std::vector<AbilityFallbackReactionProfileDto> fallbackReactionProfiles;
	};

	template<typename TEnum, typename TParser>
	bool ReadTypedString(
		const json& node,
		const char* field,
		TEnum& outValue,
		const char* typeName,
		TParser parser,
		std::string& outError)
	{
		std::string text;
		if (!ReadRequiredString(node, field, text, outError))
			return false;

		if (!parser(text, outValue))
		{
			outError = std::string("Unknown ") + typeName + ": " + text;
			return false;
		}

		return true;
	}

	bool ReadNullableString(
		const json& node,
		const char* field,
		std::optional<std::string>& outValue,
		std::string& outError)
	{
		if (!node.contains(field) || node.at(field).is_null())
		{
			outValue = std::nullopt;
			return true;
		}

		if (!node.at(field).is_string())
		{
			outError = std::string("Invalid string field: ") + field;
			return false;
		}

		outValue = node.at(field).get<std::string>();
		return true;
	}

	bool ParseRequestSemantic(
		std::string_view text,
		AbilityRequestSemantic& outValue) noexcept
	{
		if (text == "LightAttack") { outValue = AbilityRequestSemantic::LightAttack; return true; }
		if (text == "HeavyAttack") { outValue = AbilityRequestSemantic::HeavyAttack; return true; }
		if (text == "Dodge") { outValue = AbilityRequestSemantic::Dodge; return true; }
		if (text == "Parry") { outValue = AbilityRequestSemantic::Parry; return true; }
		if (text == "GuardStart") { outValue = AbilityRequestSemantic::GuardStart; return true; }
		if (text == "UseItem") { outValue = AbilityRequestSemantic::UseItem; return true; }
		if (text == "None") { outValue = AbilityRequestSemantic::None; return true; }
		return false;
	}

	bool ParseSelectionPolicy(
		std::string_view text,
		AbilityCandidateSelectionPolicy& outValue) noexcept
	{
		if (text == "OrderedFirstValid") { outValue = AbilityCandidateSelectionPolicy::OrderedFirstValid; return true; }
		if (text == "HighestPriorityValid") { outValue = AbilityCandidateSelectionPolicy::HighestPriorityValid; return true; }
		return false;
	}

	bool ReadOptionalBool(
		const json& node,
		const char* field,
		bool& outValue,
		std::string& outError)
	{
		if (!node.contains(field))
			return true;

		if (!node.at(field).is_boolean())
		{
			outError = std::string("Invalid bool field: ") + field;
			return false;
		}

		outValue = node.at(field).get<bool>();
		return true;
	}

	bool ParseTransitionCause(
		std::string_view text,
		AbilityTransitionCause& outValue) noexcept
	{
		if (text == "OnHitReceived") { outValue = AbilityTransitionCause::OnHitReceived; return true; }
		if (text == "OnParried") { outValue = AbilityTransitionCause::OnParried; return true; }
		if (text == "OnAttributeZero") { outValue = AbilityTransitionCause::OnAttributeZero; return true; }
		if (text == "HoldRelease") { outValue = AbilityTransitionCause::HoldRelease; return true; }
		if (text == "ManualCancel") { outValue = AbilityTransitionCause::ManualCancel; return true; }
		if (text == "Combo") { outValue = AbilityTransitionCause::Combo; return true; }
		if (text == "LightAttackCancel") { outValue = AbilityTransitionCause::LightAttackCancel; return true; }
		if (text == "HeavyAttackCancel") { outValue = AbilityTransitionCause::HeavyAttackCancel; return true; }
		if (text == "DodgeCancel") { outValue = AbilityTransitionCause::DodgeCancel; return true; }
		if (text == "ParryCancel") { outValue = AbilityTransitionCause::ParryCancel; return true; }
		if (text == "None") { outValue = AbilityTransitionCause::None; return true; }
		return false;
	}

	bool ParseStringArray(
		const json& node,
		std::vector<std::string>& outValues,
		std::string& outError)
	{
		if (!node.is_array())
		{
			outError = "Expected string array.";
			return false;
		}

		outValues.clear();
		outValues.reserve(node.size());
		for (const json& item : node)
		{
			if (!item.is_string())
			{
				outError = "String array entry must be a string.";
				return false;
			}

			outValues.push_back(item.get<std::string>());
		}

		return true;
	}

	template<typename TDto, typename TParser>
	bool ParseArray(
		const json& node,
		const char* field,
		std::vector<TDto>& outDtos,
		TParser parser,
		std::string& outError)
	{
		outDtos.clear();
		if (!node.contains(field))
			return true;

		if (!node.at(field).is_array())
		{
			outError = std::string("Invalid array field: ") + field;
			return false;
		}

		for (const json& item : node.at(field))
		{
			TDto dto{};
			if (!parser(item, dto, outError))
				return false;

			outDtos.push_back(std::move(dto));
		}

		return true;
	}

	bool ParseGrant(
		const json& node,
		AbilityGrantDto& outDto,
		std::string& outError)
	{
		return
			ReadRequiredString(node, "abilityKey", outDto.abilityKey, outError) &&
			ReadRequiredNumber(node, "level", outDto.level, outError);
	}

	bool ParseInputBindingEntry(
		const json& node,
		AbilityInputBindingEntryDto& outDto,
		std::string& outError)
	{
		return
			ReadTypedString(
				node,
				"request",
				outDto.request,
				"AbilityRequestSemantic",
				ParseRequestSemantic,
				outError) &&
			node.contains("candidateAbilityKeys") &&
			ParseStringArray(
				node.at("candidateAbilityKeys"),
				outDto.candidateAbilityKeys,
				outError) &&
			ReadTypedString(
				node,
				"selectionPolicy",
				outDto.selectionPolicy,
				"AbilityCandidateSelectionPolicy",
				ParseSelectionPolicy,
				outError) &&
			ReadRequiredNumber(node, "priority", outDto.priority, outError);
	}

	bool ParseInputBindingProfile(
		const json& node,
		AbilityInputBindingProfileDto& outDto,
		std::string& outError)
	{
		return
			ReadRequiredString(node, "key", outDto.key, outError) &&
			ParseArray<AbilityInputBindingEntryDto>(
				node,
				"entries",
				outDto.entries,
				ParseInputBindingEntry,
				outError);
	}

	bool ParseAnimationBinding(
		const json& node,
		AbilityAnimationBindingDto& outDto,
		std::string& outError)
	{
		return
			ReadRequiredString(node, "abilityKey", outDto.abilityKey, outError) &&
			ReadRequiredString(
				node,
				"animationKey",
				outDto.animationKey,
				outError) &&
			ReadRequiredNumber(node, "playRate", outDto.playRate, outError) &&
			ReadRequiredNumber(
				node,
				"startNormalizedTime",
				outDto.startNormalizedTime,
				outError);
	}

	bool ParseLocomotionAnimationBinding(
		const json& node,
		AbilityLocomotionAnimationBindingDto& outDto,
		std::string& outError)
	{
		return
			ReadTypedString(
				node,
				"mode",
				outDto.mode,
				"LocomotionMode",
				[](std::string_view text, LocomotionMode& value)
				{
					return ParseDefString(text, value);
				},
				outError) &&
			ReadRequiredString(
				node,
				"animationKey",
				outDto.animationKey,
				outError) &&
			ReadOptionalNumber(node, "playRate", outDto.playRate, outError) &&
			ReadOptionalBool(
				node,
				"holdLastFrame",
				outDto.holdLastFrame,
				outError);
	}

	bool ParseAnimationBindingProfile(
		const json& node,
		AbilityAnimationBindingProfileDto& outDto,
		std::string& outError)
	{
		return
			ReadRequiredString(node, "key", outDto.key, outError) &&
			ParseArray<AbilityAnimationBindingDto>(
				node,
				"abilityBindings",
				outDto.abilityBindings,
				ParseAnimationBinding,
				outError) &&
			ParseArray<AbilityLocomotionAnimationBindingDto>(
				node,
				"locomotionBindings",
				outDto.locomotionBindings,
				ParseLocomotionAnimationBinding,
				outError);
	}

	bool ParseFallbackReactionEntry(
		const json& node,
		AbilityFallbackReactionEntryDto& outDto,
		std::string& outError)
	{
		return
			ReadTypedString(
				node,
				"cause",
				outDto.cause,
				"AbilityTransitionCause",
				ParseTransitionCause,
				outError) &&
			ReadRequiredString(node, "toAbilityKey", outDto.toAbilityKey, outError) &&
			ReadRequiredNumber(node, "priority", outDto.priority, outError);
	}

	bool ParseFallbackReactionProfile(
		const json& node,
		AbilityFallbackReactionProfileDto& outDto,
		std::string& outError)
	{
		return
			ReadRequiredString(node, "key", outDto.key, outError) &&
			ParseArray<AbilityFallbackReactionEntryDto>(
				node,
				"entries",
				outDto.entries,
				ParseFallbackReactionEntry,
				outError);
	}

	bool ParseAbilitySet(
		const json& node,
		AbilitySetDto& outDto,
		std::string& outError)
	{
		return
			ReadRequiredString(node, "key", outDto.key, outError) &&
			ReadRequiredString(node, "name", outDto.name, outError) &&
			ParseArray<AbilityGrantDto>(
				node,
				"grants",
				outDto.grants,
				ParseGrant,
				outError) &&
			ReadNullableString(
				node,
				"inputBindingProfileKey",
				outDto.inputBindingProfileKey,
				outError) &&
			ReadNullableString(
				node,
				"animationBindingProfileKey",
				outDto.animationBindingProfileKey,
				outError) &&
			ReadNullableString(
				node,
				"fallbackReactionProfileKey",
				outDto.fallbackReactionProfileKey,
				outError);
	}

	bool ParseAbilitySetDocument(
		const json& root,
		ParsedAbilitySetDefinitions& outDefs,
		std::string& outError)
	{
		return
			ParseArray<AbilitySetDto>(
				root,
				"abilitySets",
				outDefs.abilitySets,
				ParseAbilitySet,
				outError) &&
			ParseArray<AbilityInputBindingProfileDto>(
				root,
				"inputBindingProfiles",
				outDefs.inputBindingProfiles,
				ParseInputBindingProfile,
				outError) &&
			ParseArray<AbilityAnimationBindingProfileDto>(
				root,
				"animationBindingProfiles",
				outDefs.animationBindingProfiles,
				ParseAnimationBindingProfile,
				outError) &&
			ParseArray<AbilityFallbackReactionProfileDto>(
				root,
				"fallbackReactionProfiles",
				outDefs.fallbackReactionProfiles,
				ParseFallbackReactionProfile,
				outError);
	}

	bool AppendAbilitySetDocumentDtos(
		const json& root,
		std::vector<ParsedAbilitySetDefinitions>& outDtos,
		std::string& outError)
	{
		ParsedAbilitySetDefinitions defs{};
		if (!ParseAbilitySetDocument(root, defs, outError))
			return false;

		outDtos.push_back(std::move(defs));
		return true;
	}

	void AppendAbilitySetDefinitions(
		ParsedAbilitySetDefinitions& dst,
		ParsedAbilitySetDefinitions&& src)
	{
		dst.abilitySets.insert(
			dst.abilitySets.end(),
			std::make_move_iterator(src.abilitySets.begin()),
			std::make_move_iterator(src.abilitySets.end()));
		dst.inputBindingProfiles.insert(
			dst.inputBindingProfiles.end(),
			std::make_move_iterator(src.inputBindingProfiles.begin()),
			std::make_move_iterator(src.inputBindingProfiles.end()));
		dst.animationBindingProfiles.insert(
			dst.animationBindingProfiles.end(),
			std::make_move_iterator(src.animationBindingProfiles.begin()),
			std::make_move_iterator(src.animationBindingProfiles.end()));
		dst.fallbackReactionProfiles.insert(
			dst.fallbackReactionProfiles.end(),
			std::make_move_iterator(src.fallbackReactionProfiles.begin()),
			std::make_move_iterator(src.fallbackReactionProfiles.end()));
	}

	template<typename TDto, typename TId, typename TKeyGetter>
	bool BuildKeyIndex(
		std::span<const TDto> dtos,
		TId firstRuntimeId,
		TKeyGetter getKey,
		DefKeyIndex<TId>& outIndex,
		std::string& outError)
	{
		std::vector<std::string> keys;
		keys.reserve(dtos.size());
		for (const TDto& dto : dtos)
			keys.push_back(getKey(dto));

		return BuildDeterministicDefKeyIndex(
			std::span<const std::string>(keys.data(), keys.size()),
			firstRuntimeId,
			outIndex,
			outError);
	}

	bool ResolveAbilityIdByKey(
		std::span<const AbilityDef> abilities,
		std::string_view key,
		AbilityId& outId,
		std::string& outError)
	{
		for (const AbilityDef& ability : abilities)
		{
			if (ability.key == key)
			{
				outId = ability.id;
				return true;
			}
		}

		outError = "Unknown ability key: " + std::string(key);
		return false;
	}

	bool CompileGrant(
		const AbilityGrantDto& dto,
		std::span<const AbilityDef> abilities,
		AbilityGrantDef& outDef,
		std::string& outError)
	{
		if (!ResolveAbilityIdByKey(
				abilities,
				dto.abilityKey,
				outDef.abilityId,
				outError))
		{
			return false;
		}

		outDef.level = dto.level;
		return true;
	}

	bool CompileInputBindingEntry(
		const AbilityInputBindingEntryDto& dto,
		std::span<const AbilityDef> abilities,
		AbilityInputBindingEntryDef& outDef,
		std::string& outError)
	{
		outDef.request = dto.request;
		outDef.selectionPolicy = dto.selectionPolicy;
		outDef.priority = dto.priority;
		outDef.candidateAbilities.clear();
		outDef.candidateAbilities.reserve(dto.candidateAbilityKeys.size());

		for (const std::string& abilityKey : dto.candidateAbilityKeys)
		{
			AbilityId abilityId{ InvalidAbilityId };
			if (!ResolveAbilityIdByKey(
					abilities,
					abilityKey,
					abilityId,
					outError))
			{
				return false;
			}

			outDef.candidateAbilities.push_back(abilityId);
		}

		return true;
	}

	bool CompileInputBindingProfile(
		const AbilityInputBindingProfileDto& dto,
		const DefKeyIndex<AbilityInputBindingProfileId>& keyIndex,
		std::span<const AbilityDef> abilities,
		AbilityInputBindingProfileDef& outDef,
		std::string& outError)
	{
		if (!keyIndex.FindId(dto.key, outDef.id))
		{
			outError = "Unknown ability input binding profile key: " + dto.key;
			return false;
		}

		outDef.key = dto.key;
		outDef.entries.clear();
		outDef.entries.reserve(dto.entries.size());
		for (const AbilityInputBindingEntryDto& entryDto : dto.entries)
		{
			AbilityInputBindingEntryDef entry{};
			if (!CompileInputBindingEntry(entryDto, abilities, entry, outError))
				return false;

			outDef.entries.push_back(std::move(entry));
		}

		return true;
	}

	bool CompileAnimationBinding(
		const AbilityAnimationBindingDto& dto,
		std::span<const AbilityDef> abilities,
		AbilityAnimationBindingDef& outDef,
		std::string& outError)
	{
		if (!ResolveAbilityIdByKey(
				abilities,
				dto.abilityKey,
				outDef.abilityId,
				outError))
		{
			return false;
		}

		outDef.animationKey = dto.animationKey;
		outDef.playRate = dto.playRate;
		outDef.startNormalizedTime = dto.startNormalizedTime;
		return true;
	}

	bool CompileLocomotionAnimationBinding(
		const AbilityLocomotionAnimationBindingDto& dto,
		AbilityLocomotionAnimationBindingDef& outDef,
		std::string& outError)
	{
		if (dto.animationKey.empty())
		{
			outError = "Ability locomotion animation binding key must not be empty.";
			return false;
		}

		outDef.mode = dto.mode;
		outDef.animationKey = dto.animationKey;
		outDef.playRate = dto.playRate;
		outDef.holdLastFrame = dto.holdLastFrame;
		return true;
	}

	bool CompileAnimationBindingProfile(
		const AbilityAnimationBindingProfileDto& dto,
		const DefKeyIndex<AbilityAnimationBindingProfileId>& keyIndex,
		std::span<const AbilityDef> abilities,
		AbilityAnimationBindingProfileDef& outDef,
		std::string& outError)
	{
		if (!keyIndex.FindId(dto.key, outDef.id))
		{
			outError = "Unknown ability animation binding profile key: " +
				dto.key;
			return false;
		}

		outDef.key = dto.key;
		outDef.abilityBindings.clear();
		outDef.abilityBindings.reserve(dto.abilityBindings.size());
		for (const AbilityAnimationBindingDto& bindingDto :
			dto.abilityBindings)
		{
			AbilityAnimationBindingDef binding{};
			if (!CompileAnimationBinding(bindingDto, abilities, binding, outError))
				return false;

			outDef.abilityBindings.push_back(std::move(binding));
		}

		outDef.locomotionBindings.clear();
		outDef.locomotionBindings.reserve(dto.locomotionBindings.size());
		for (const AbilityLocomotionAnimationBindingDto& bindingDto :
			dto.locomotionBindings)
		{
			AbilityLocomotionAnimationBindingDef binding{};
			if (!CompileLocomotionAnimationBinding(
					bindingDto,
					binding,
					outError))
			{
				return false;
			}

			outDef.locomotionBindings.push_back(std::move(binding));
		}

		return true;
	}

	bool CompileFallbackReactionEntry(
		const AbilityFallbackReactionEntryDto& dto,
		std::span<const AbilityDef> abilities,
		AbilityFallbackReactionEntryDef& outDef,
		std::string& outError)
	{
		if (!ResolveAbilityIdByKey(
				abilities,
				dto.toAbilityKey,
				outDef.toAbilityId,
				outError))
		{
			return false;
		}

		outDef.cause = dto.cause;
		outDef.priority = dto.priority;
		return true;
	}

	bool CompileFallbackReactionProfile(
		const AbilityFallbackReactionProfileDto& dto,
		const DefKeyIndex<AbilityFallbackReactionProfileId>& keyIndex,
		std::span<const AbilityDef> abilities,
		AbilityFallbackReactionProfileDef& outDef,
		std::string& outError)
	{
		if (!keyIndex.FindId(dto.key, outDef.id))
		{
			outError = "Unknown ability fallback reaction profile key: " +
				dto.key;
			return false;
		}

		outDef.key = dto.key;
		outDef.entries.clear();
		outDef.entries.reserve(dto.entries.size());
		for (const AbilityFallbackReactionEntryDto& entryDto : dto.entries)
		{
			AbilityFallbackReactionEntryDef entry{};
			if (!CompileFallbackReactionEntry(entryDto, abilities, entry, outError))
				return false;

			outDef.entries.push_back(std::move(entry));
		}

		return true;
	}

	bool CompileAbilitySet(
		const AbilitySetDto& dto,
		const DefKeyIndex<AbilitySetId>& setKeyIndex,
		const DefKeyIndex<AbilityInputBindingProfileId>& inputKeyIndex,
		const DefKeyIndex<AbilityAnimationBindingProfileId>& animationKeyIndex,
		const DefKeyIndex<AbilityFallbackReactionProfileId>& fallbackKeyIndex,
		std::span<const AbilityDef> abilities,
		AbilitySetDef& outDef,
		std::string& outError)
	{
		if (!setKeyIndex.FindId(dto.key, outDef.id))
		{
			outError = "Unknown ability set key: " + dto.key;
			return false;
		}

		outDef.key = dto.key;
		outDef.name = dto.name;
		outDef.grants.clear();
		outDef.grants.reserve(dto.grants.size());
		for (const AbilityGrantDto& grantDto : dto.grants)
		{
			AbilityGrantDef grant{};
			if (!CompileGrant(grantDto, abilities, grant, outError))
				return false;

			outDef.grants.push_back(std::move(grant));
		}

		outDef.inputBindingProfileId = std::nullopt;
		if (dto.inputBindingProfileKey.has_value())
		{
			AbilityInputBindingProfileId id{
				InvalidAbilityInputBindingProfileId };
			if (!inputKeyIndex.FindId(*dto.inputBindingProfileKey, id))
			{
				outError = "Unknown ability input binding profile key: " +
					*dto.inputBindingProfileKey;
				return false;
			}

			outDef.inputBindingProfileId = id;
		}

		outDef.animationBindingProfileId = std::nullopt;
		if (dto.animationBindingProfileKey.has_value())
		{
			AbilityAnimationBindingProfileId id{
				InvalidAbilityAnimationBindingProfileId };
			if (!animationKeyIndex.FindId(*dto.animationBindingProfileKey, id))
			{
				outError = "Unknown ability animation binding profile key: " +
					*dto.animationBindingProfileKey;
				return false;
			}

			outDef.animationBindingProfileId = id;
		}

		outDef.fallbackReactionProfileId = std::nullopt;
		if (dto.fallbackReactionProfileKey.has_value())
		{
			AbilityFallbackReactionProfileId id{
				InvalidAbilityFallbackReactionProfileId };
			if (!fallbackKeyIndex.FindId(*dto.fallbackReactionProfileKey, id))
			{
				outError = "Unknown ability fallback reaction profile key: " +
					*dto.fallbackReactionProfileKey;
				return false;
			}

			outDef.fallbackReactionProfileId = id;
		}

		return true;
	}

	bool BuildRegistries(
		const ParsedAbilitySetDefinitions& dtos,
		std::span<const AbilityDef> abilities,
		AbilitySetDefinitionRegistries& outRegistries,
		std::string& outError)
	{
		DefKeyIndex<AbilitySetId> setKeyIndex;
		DefKeyIndex<AbilityInputBindingProfileId> inputKeyIndex;
		DefKeyIndex<AbilityAnimationBindingProfileId> animationKeyIndex;
		DefKeyIndex<AbilityFallbackReactionProfileId> fallbackKeyIndex;

		if (!BuildKeyIndex<AbilitySetDto>(
				std::span<const AbilitySetDto>(
					dtos.abilitySets.data(),
					dtos.abilitySets.size()),
				static_cast<AbilitySetId>(InvalidAbilitySetId + 1),
				[](const AbilitySetDto& dto) -> const std::string&
				{
					return dto.key;
				},
				setKeyIndex,
				outError) ||
			!BuildKeyIndex<AbilityInputBindingProfileDto>(
				std::span<const AbilityInputBindingProfileDto>(
					dtos.inputBindingProfiles.data(),
					dtos.inputBindingProfiles.size()),
				static_cast<AbilityInputBindingProfileId>(
					InvalidAbilityInputBindingProfileId + 1),
				[](const AbilityInputBindingProfileDto& dto) ->
					const std::string&
				{
					return dto.key;
				},
				inputKeyIndex,
				outError) ||
			!BuildKeyIndex<AbilityAnimationBindingProfileDto>(
				std::span<const AbilityAnimationBindingProfileDto>(
					dtos.animationBindingProfiles.data(),
					dtos.animationBindingProfiles.size()),
				static_cast<AbilityAnimationBindingProfileId>(
					InvalidAbilityAnimationBindingProfileId + 1),
				[](const AbilityAnimationBindingProfileDto& dto) ->
					const std::string&
				{
					return dto.key;
				},
				animationKeyIndex,
				outError) ||
			!BuildKeyIndex<AbilityFallbackReactionProfileDto>(
				std::span<const AbilityFallbackReactionProfileDto>(
					dtos.fallbackReactionProfiles.data(),
					dtos.fallbackReactionProfiles.size()),
				static_cast<AbilityFallbackReactionProfileId>(
					InvalidAbilityFallbackReactionProfileId + 1),
				[](const AbilityFallbackReactionProfileDto& dto) ->
					const std::string&
				{
					return dto.key;
				},
				fallbackKeyIndex,
				outError))
		{
			return false;
		}

		std::vector<AbilityInputBindingProfileDef> inputProfiles;
		inputProfiles.reserve(dtos.inputBindingProfiles.size());
		for (const AbilityInputBindingProfileDto& dto :
			dtos.inputBindingProfiles)
		{
			AbilityInputBindingProfileDef def{};
			if (!CompileInputBindingProfile(
					dto,
					inputKeyIndex,
					abilities,
					def,
					outError))
			{
				return false;
			}

			inputProfiles.push_back(std::move(def));
		}

		std::vector<AbilityAnimationBindingProfileDef> animationProfiles;
		animationProfiles.reserve(dtos.animationBindingProfiles.size());
		for (const AbilityAnimationBindingProfileDto& dto :
			dtos.animationBindingProfiles)
		{
			AbilityAnimationBindingProfileDef def{};
			if (!CompileAnimationBindingProfile(
					dto,
					animationKeyIndex,
					abilities,
					def,
					outError))
			{
				return false;
			}

			animationProfiles.push_back(std::move(def));
		}

		std::vector<AbilityFallbackReactionProfileDef> fallbackProfiles;
		fallbackProfiles.reserve(dtos.fallbackReactionProfiles.size());
		for (const AbilityFallbackReactionProfileDto& dto :
			dtos.fallbackReactionProfiles)
		{
			AbilityFallbackReactionProfileDef def{};
			if (!CompileFallbackReactionProfile(
					dto,
					fallbackKeyIndex,
					abilities,
					def,
					outError))
			{
				return false;
			}

			fallbackProfiles.push_back(std::move(def));
		}

		std::vector<AbilitySetDef> sets;
		sets.reserve(dtos.abilitySets.size());
		for (const AbilitySetDto& dto : dtos.abilitySets)
		{
			AbilitySetDef def{};
			if (!CompileAbilitySet(
					dto,
					setKeyIndex,
					inputKeyIndex,
					animationKeyIndex,
					fallbackKeyIndex,
					abilities,
					def,
					outError))
			{
				return false;
			}

			sets.push_back(std::move(def));
		}

		std::sort(
			sets.begin(),
			sets.end(),
			[](const AbilitySetDef& lhs, const AbilitySetDef& rhs)
			{
				return lhs.id < rhs.id;
			});
		std::sort(
			inputProfiles.begin(),
			inputProfiles.end(),
			[](const auto& lhs, const auto& rhs)
			{
				return lhs.id < rhs.id;
			});
		std::sort(
			animationProfiles.begin(),
			animationProfiles.end(),
			[](const auto& lhs, const auto& rhs)
			{
				return lhs.id < rhs.id;
			});
		std::sort(
			fallbackProfiles.begin(),
			fallbackProfiles.end(),
			[](const auto& lhs, const auto& rhs)
			{
				return lhs.id < rhs.id;
			});

		if (!ValidateAbilitySetDefs(
				std::span<const AbilitySetDef>(sets.data(), sets.size()),
				abilities,
				std::span<const AbilityInputBindingProfileDef>(
					inputProfiles.data(),
					inputProfiles.size()),
				std::span<const AbilityAnimationBindingProfileDef>(
					animationProfiles.data(),
					animationProfiles.size()),
				std::span<const AbilityFallbackReactionProfileDef>(
					fallbackProfiles.data(),
					fallbackProfiles.size()),
				outError))
		{
			return false;
		}

		AbilitySetDefinitionRegistries registries;
		if (!registries.abilitySets.Build(std::move(sets), &outError) ||
			!registries.inputBindingProfiles.Build(
				std::move(inputProfiles),
				&outError) ||
			!registries.animationBindingProfiles.Build(
				std::move(animationProfiles),
				&outError) ||
			!registries.fallbackReactionProfiles.Build(
				std::move(fallbackProfiles),
				&outError))
		{
			return false;
		}

		outRegistries = std::move(registries);
		return true;
	}
}

DefLoadResult LoadAbilitySetDefsFromJsonDirectory(
	const std::filesystem::path& directory,
	std::span<const AbilityDef> abilities,
	AbilitySetDefinitionRegistries& outRegistries)
{
	DefLoadResult result{};
	std::vector<DefJsonDocument> documents;
	result = LoadDefJsonDocumentsFromDirectory(
		directory,
		documents,
		"AbilitySet");
	if (!result.succeeded)
		return result;

	DefParsedDtoList<ParsedAbilitySetDefinitions> dtos;
	if (!ParseDefDtosFromJsonDocuments(
			std::span<const DefJsonDocument>(documents.data(), documents.size()),
			AppendAbilitySetDocumentDtos,
			dtos,
			result.error))
	{
		result.succeeded = false;
		return result;
	}

	ParsedAbilitySetDefinitions mergedDefs{};
	for (DefParsedDto<ParsedAbilitySetDefinitions>& parsedDto : dtos)
	{
		AppendAbilitySetDefinitions(mergedDefs, std::move(parsedDto.dto));
	}

	if (!BuildRegistries(mergedDefs, abilities, outRegistries, result.error))
	{
		result.succeeded = false;
		return result;
	}

	result.succeeded = true;
	result.loadedCount =
		outRegistries.abilitySets.Size() +
		outRegistries.inputBindingProfiles.Size() +
		outRegistries.animationBindingProfiles.Size() +
		outRegistries.fallbackReactionProfiles.Size();
	result.error.clear();
	return result;
}
