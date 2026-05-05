#pragma once

#include "DefRegistry.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

template<typename TId>
struct DefKeyEntry
{
	std::string key;
	TId id{};
};

template<typename TId>
class DefKeyIndex
{
public:
	bool Build(
		std::vector<DefKeyEntry<TId>> entries,
		std::string* outError = nullptr)
	{
		Clear();

		_entries = std::move(entries);
		_idIndex.reserve(_entries.size());
		_keyIndex.reserve(_entries.size());

		for (size_t index = 0; index < _entries.size(); ++index)
		{
			const DefKeyEntry<TId>& entry = _entries[index];
			if (entry.key.empty())
			{
				if (outError != nullptr)
					*outError = "Def key must not be empty.";

				Clear();
				return false;
			}

			if (entry.id == TId{})
			{
				if (outError != nullptr)
					*outError = "Def id must not be invalid.";

				Clear();
				return false;
			}

			if (!_keyIndex.try_emplace(entry.key, index).second)
			{
				if (outError != nullptr)
					*outError = "Duplicate def key detected: " + entry.key;

				Clear();
				return false;
			}

			if (!_idIndex.try_emplace(entry.id, index).second)
			{
				if (outError != nullptr)
					*outError = "Duplicate def id detected.";

				Clear();
				return false;
			}
		}

		return true;
	}

	void Clear() noexcept
	{
		_entries.clear();
		_keyIndex.clear();
		_idIndex.clear();
	}

	bool FindId(std::string_view key, TId& outId) const
	{
		const auto it = _keyIndex.find(std::string(key));
		if (it == _keyIndex.end())
			return false;

		outId = _entries[it->second].id;
		return true;
	}

	const std::string* FindKey(TId id) const noexcept
	{
		const auto it = _idIndex.find(id);
		if (it == _idIndex.end())
			return nullptr;

		return &_entries[it->second].key;
	}

	std::span<const DefKeyEntry<TId>> GetAll() const noexcept
	{
		return std::span<const DefKeyEntry<TId>>(_entries);
	}

private:
	std::vector<DefKeyEntry<TId>> _entries;
	std::unordered_map<std::string, size_t> _keyIndex;
	std::unordered_map<TId, size_t, DefRegistryIdHash<TId>> _idIndex;
};

template<typename TId, bool IsEnum = std::is_enum_v<TId>>
struct DefKeyNumericId
{
	using Type = TId;
};

template<typename TId>
struct DefKeyNumericId<TId, true>
{
	using Type = std::underlying_type_t<TId>;
};

template<typename TId>
bool BuildDeterministicDefKeyIndex(
	std::span<const std::string> keys,
	TId firstRuntimeId,
	DefKeyIndex<TId>& outIndex,
	std::string& outError)
{
	using NumericId = typename DefKeyNumericId<TId>::Type;

	static_assert(
		std::is_integral_v<NumericId>,
		"Def runtime id must be an integral or enum type.");

	std::vector<std::string> sortedKeys(keys.begin(), keys.end());
	std::sort(sortedKeys.begin(), sortedKeys.end());

	for (const std::string& key : sortedKeys)
	{
		if (key.empty())
		{
			outError = "Def key must not be empty.";
			return false;
		}
	}

	const auto duplicate = std::adjacent_find(
		sortedKeys.begin(),
		sortedKeys.end());
	if (duplicate != sortedKeys.end())
	{
		outError = "Duplicate def key detected: " + *duplicate;
		return false;
	}

	const uint64_t firstValue =
		static_cast<uint64_t>(static_cast<NumericId>(firstRuntimeId));
	const uint64_t maxValue =
		static_cast<uint64_t>(std::numeric_limits<NumericId>::max());
	if (!sortedKeys.empty() &&
		firstValue + sortedKeys.size() - 1 > maxValue)
	{
		outError = "Def key count exceeds runtime id range.";
		return false;
	}

	std::vector<DefKeyEntry<TId>> entries;
	entries.reserve(sortedKeys.size());
	for (size_t index = 0; index < sortedKeys.size(); ++index)
	{
		const NumericId runtimeId =
			static_cast<NumericId>(firstValue + index);
		entries.push_back(DefKeyEntry<TId>{
			.key = std::move(sortedKeys[index]),
			.id = static_cast<TId>(runtimeId)
		});
	}

	return outIndex.Build(std::move(entries), &outError);
}
