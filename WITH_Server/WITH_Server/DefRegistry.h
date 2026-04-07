#pragma once

#include <cstddef>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

template<typename TId>
struct DefRegistryIdHash
{
	size_t operator()(TId id) const noexcept
	{
		if constexpr (std::is_enum_v<TId>)
		{
			using Underlying = std::underlying_type_t<TId>;
			return std::hash<Underlying>{}(static_cast<Underlying>(id));
		}
		else
		{
			return std::hash<TId>{}(id);
		}
	}
};

template<typename TDef, typename TId, typename TTraits, typename THash = DefRegistryIdHash<TId>>
class DefRegistry {
public:
	bool Build(std::vector<TDef> defs, std::string* outError = nullptr)
	{
		Clear();

		_defs = std::move(defs);
		_indexById.reserve(_defs.size());

		for (size_t index = 0; index < _defs.size(); ++index)
		{
			const TId id = TTraits::GetId(_defs[index]);
			const auto [it, inserted] = _indexById.try_emplace(id, index);
			(void)it;

			if (!inserted)
			{
				if (outError != nullptr)
				{
					*outError = "Duplicate def id detected.";
				}

				Clear();
				return false;
			}
		}

		return true;
	}

	void Clear() noexcept
	{
		_defs.clear();
		_indexById.clear();
	}

	bool Empty() const noexcept
	{
		return _defs.empty();
	}

	size_t Size() const noexcept
	{
		return _defs.size();
	}

	const TDef* Find(TId id) const noexcept
	{
		const auto it = _indexById.find(id);
		if (it == _indexById.end())
		{
			return nullptr;
		}

		return &_defs[it->second];
	}

	const TDef& Get(TId id) const
	{
		const TDef* const def = Find(id);
		if (def == nullptr)
		{
			throw std::out_of_range("DefRegistry entry was not found.");
		}

		return *def;
	}

	std::span<const TDef> GetAll() const noexcept
	{
		return std::span<const TDef>(_defs);
	}

private:
	std::vector<TDef> _defs;
	std::unordered_map<TId, size_t, THash> _indexById;
};
