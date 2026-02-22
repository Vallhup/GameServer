#pragma once

#include "types.h"

class WorldId {
public:
	constexpr WorldId() : _value(0) {}
	static constexpr WorldId Invalid() { return WorldId(); }

	static constexpr WorldId Create(uint32 id, uint32 gen)
	{
		WorldId out;
		out._value = (uint64(gen) << 32) | uint64(id);
		return out;
	}

	constexpr bool IsValid() const { return _value != 0; }

	constexpr uint32 GetId() const { return uint32(_value & 0xFFFFFFFF); }
	constexpr uint32 GetGen() const { return uint32((_value >> 32) & 0xFFFFFFFF); }
	constexpr uint64 GetRaw() const { return _value; }

	constexpr std::strong_ordering operator<=>(const WorldId& rhs) const { return _value <=> rhs._value; }
	constexpr bool operator==(const WorldId& rhs) const { return _value == rhs._value; }

private:
	uint64 _value;
};

namespace std {
	template<>
	struct hash<WorldId> {
		size_t operator()(const WorldId& id) const noexcept
		{
			return std::hash<uint64>()(id.GetRaw());
		}
	};
}
