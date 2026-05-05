#pragma once

#include "AbilitySetDef.h"
#include "AttributeDef.h"
#include "GameplayEffectDef.h"
#include "GameplayTagDef.h"

#include <filesystem>
#include <stdexcept>
#include <string_view>

struct GameplayContentCatalogRoots
{
	std::filesystem::path attributeRoot;
	std::filesystem::path tagRoot;
	std::filesystem::path effectRoot;
	std::filesystem::path abilityRoot;
	std::filesystem::path abilitySetRoot;
};

class GameplayContentCatalogSnapshot final
{
public:
	static const GameplayContentCatalogSnapshot& Current()
	{
		const GameplayContentCatalogSnapshot* catalog = TryCurrent();
		if (catalog == nullptr)
		{
			throw std::runtime_error(
				"GameplayContentCatalog current snapshot is not published.");
		}

		return *catalog;
	}

	static const GameplayContentCatalogSnapshot* TryCurrent() noexcept
	{
		return CurrentSlot();
	}

	static void Publish(const GameplayContentCatalogSnapshot& catalog) noexcept
	{
		CurrentSlot() = &catalog;
	}

	static void ClearCurrent() noexcept
	{
		CurrentSlot() = nullptr;
	}

	DefLoadResult LoadFromRoots(const GameplayContentCatalogRoots& roots);

	const AttributeDefRegistry& Attributes() const noexcept
	{
		return _attributes;
	}

	const GameplayTagDefRegistry& GameplayTags() const noexcept
	{
		return _tags;
	}

	const GameplayEffectDefRegistry& GameplayEffects() const noexcept
	{
		return _effects;
	}

	const AbilityDefRegistry& Abilities() const noexcept
	{
		return _abilities;
	}

	const AbilitySetDefinitionRegistries& AbilitySets() const noexcept
	{
		return _abilitySets;
	}

	const AbilityDef* FindAbilityByKey(std::string_view key) const noexcept;
	const AbilitySetDef* FindAbilitySetByKey(std::string_view key) const noexcept;
	const AttributeDef* FindAttributeByKey(std::string_view key) const noexcept;

	void Clear() noexcept;
	size_t Size() const noexcept;

private:
	static const GameplayContentCatalogSnapshot*& CurrentSlot() noexcept
	{
		static const GameplayContentCatalogSnapshot* catalog{ nullptr };
		return catalog;
	}

	AttributeDefRegistry _attributes;
	GameplayTagDefRegistry _tags;
	GameplayEffectDefRegistry _effects;
	AbilityDefRegistry _abilities;
	AbilitySetDefinitionRegistries _abilitySets;
};
