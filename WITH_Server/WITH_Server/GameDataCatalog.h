#pragma once

#include "AIBehaviorDef.h"
#include "CharacterDef.h"
#include "SpawnSetDef.h"
#include "TitleDef.h"

#include <cstddef>
#include <filesystem>
#include <span>
#include <stdexcept>
#include <utility>

template<typename TDef, typename TId>
class IReadOnlyDefRepository {
public:
	virtual ~IReadOnlyDefRepository() = default;

	virtual const TDef* Find(TId id) const noexcept = 0;
	virtual const TDef& Get(TId id) const = 0;
	virtual std::span<const TDef> GetAll() const noexcept = 0;
};

template<typename TRegistry, typename TDef, typename TId>
class DefRegistryRepository final :
	public IReadOnlyDefRepository<TDef, TId> {
public:
	DefRegistryRepository() = default;

	explicit DefRegistryRepository(const TRegistry* registry) noexcept
		: _registry(registry)
	{
	}

	void Reset(const TRegistry* registry) noexcept
	{
		_registry = registry;
	}

	const TDef* Find(TId id) const noexcept override
	{
		return _registry != nullptr ? _registry->Find(id) : nullptr;
	}

	const TDef& Get(TId id) const override
	{
		return _registry->Get(id);
	}

	std::span<const TDef> GetAll() const noexcept override
	{
		return _registry != nullptr ?
			_registry->GetAll() :
			std::span<const TDef>{};
	}

private:
	const TRegistry* _registry{ nullptr };
};

class IAIBehaviorRepository {
public:
	virtual ~IAIBehaviorRepository() = default;

	virtual const AIBehaviorProfileDef* Find(
		AIBehaviorProfileId profileId) const noexcept = 0;

	virtual std::span<const AIBehaviorProfileDef> GetAll() const noexcept = 0;
};

struct GameDataCatalogRoots
{
	std::filesystem::path characterRoot;
	std::filesystem::path aiBehaviorRoot;
	std::filesystem::path spawnSetRoot;
	std::filesystem::path titleRoot;
};

class AIBehaviorRepository final : public IAIBehaviorRepository {
public:
	void Reset(const AIBehaviorDefinitionSet* defs) noexcept
	{
		_defs = defs;
	}

	const AIBehaviorProfileDef* Find(
		AIBehaviorProfileId profileId) const noexcept override
	{
		if (_defs == nullptr)
			return nullptr;

		return _defs->profiles.Find(profileId);
	}

	std::span<const AIBehaviorProfileDef> GetAll() const noexcept override
	{
		return _defs != nullptr ?
			_defs->profiles.GetAll() :
			std::span<const AIBehaviorProfileDef>{};
	}

private:
	const AIBehaviorDefinitionSet* _defs{ nullptr };
};

class GameDataCatalog final {
public:
	GameDataCatalog()
	{
		ResetRepositories();
	}

	static const GameDataCatalog& Current()
	{
		const GameDataCatalog* catalog = TryCurrent();
		if (catalog == nullptr)
		{
			throw std::runtime_error(
				"GameDataCatalog current snapshot is not published.");
		}

		return *catalog;
	}

	static const GameDataCatalog* TryCurrent() noexcept
	{
		return CurrentSlot();
	}

	static void Publish(const GameDataCatalog& catalog) noexcept
	{
		CurrentSlot() = &catalog;
	}

	static void Clear() noexcept
	{
		CurrentSlot() = nullptr;
	}

	DefLoadResult LoadFromRoots(const GameDataCatalogRoots& roots)
	{
		GameDataCatalog loaded;

		DefLoadResult result = LoadCharacterDefsFromJsonDirectory(
			roots.characterRoot,
			loaded._characters);
		if (!result.succeeded)
		{
			result.error = "Character catalog load failed: " + result.error;
			return result;
		}

		result = LoadAIBehaviorProfileDefsFromJsonDirectory(
			roots.aiBehaviorRoot,
			loaded._aiBehaviors);
		if (!result.succeeded)
		{
			result.error = "AI behavior catalog load failed: " + result.error;
			return result;
		}

		result = LoadSpawnSetDefsFromJsonDirectory(
			roots.spawnSetRoot,
			loaded._spawnSets);
		if (!result.succeeded)
		{
			result.error = "SpawnSet catalog load failed: " + result.error;
			return result;
		}

		result = LoadTitleDefsFromJsonDirectory(
			roots.titleRoot,
			loaded._titles);
		if (!result.succeeded)
		{
			result.error = "Title catalog load failed: " + result.error;
			return result;
		}

		*this = std::move(loaded);
		ResetRepositories();

		result.succeeded = true;
		result.loadedCount = Size();
		result.error.clear();
		return result;
	}

	const IAIBehaviorRepository&
		AIBehaviors() const noexcept
	{
		return _aiBehaviorRepository;
	}

	const IReadOnlyDefRepository<CharacterDef, CharacterId>&
		Characters() const noexcept
	{
		return _characterRepository;
	}

	const IReadOnlyDefRepository<SpawnSetDef, SpawnSetId>&
		SpawnSets() const noexcept
	{
		return _spawnSetRepository;
	}

	const IReadOnlyDefRepository<TitleDef, TitleId>&
		Titles() const noexcept
	{
		return _titleRepository;
	}

	size_t Size() const noexcept
	{
		return _aiBehaviors.Size() +
			_characters.Size() +
			_spawnSets.Size() +
			_titles.Size();
	}

private:
	static const GameDataCatalog*& CurrentSlot() noexcept
	{
		static const GameDataCatalog* catalog{ nullptr };
		return catalog;
	}

	void ResetRepositories() noexcept
	{
		_aiBehaviorRepository.Reset(&_aiBehaviors);
		_characterRepository.Reset(&_characters);
		_spawnSetRepository.Reset(&_spawnSets);
		_titleRepository.Reset(&_titles);
	}

	AIBehaviorDefinitionSet _aiBehaviors;
	CharacterDefRegistry    _characters;
	SpawnSetDefRegistry     _spawnSets;
	TitleDefRegistry        _titles;

	AIBehaviorRepository _aiBehaviorRepository;
	DefRegistryRepository<CharacterDefRegistry, CharacterDef, CharacterId>
		_characterRepository;
	DefRegistryRepository<SpawnSetDefRegistry, SpawnSetDef, SpawnSetId>
		_spawnSetRepository;
	DefRegistryRepository<TitleDefRegistry, TitleDef, TitleId>
		_titleRepository;
};
