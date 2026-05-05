#pragma once

#include <cstdint>
#include <string_view>

using DefHashId = uint64_t;
using AIBehaviorProfileId = DefHashId;

inline constexpr DefHashId InvalidDefHashId = 0;
inline constexpr AIBehaviorProfileId InvalidAIBehaviorProfileId = 0;

inline constexpr DefHashId HashDefKey(std::string_view key) noexcept
{
	DefHashId hash = 14695981039346656037ull;
	for (const char c : key)
	{
		hash ^= static_cast<unsigned char>(c);
		hash *= 1099511628211ull;
	}
	return hash;
}
