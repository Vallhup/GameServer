#pragma once

#include <cstdint>

class WorldId {
public:
	constexpr WorldId() : _value(0) {}
	static constexpr WorldId Invalid() { return WorldId(); }

	static constexpr WorldId Create(uint32_t id, uint32_t gen)
	{
		WorldId out;
		out._value = (uint64_t(gen) << 32) | uint64_t(id);
		return out;
	}

	constexpr bool IsValid() const { return _value != 0; }

	constexpr uint32_t GetId() const { return uint32_t(_value & 0xFFFFFFFF); }
	constexpr uint32_t GetGen() const { return uint32_t((_value >> 32) & 0xFFFFFFFF); }
	constexpr uint64_t GetRaw() const { return _value; }

	constexpr std::strong_ordering operator<=>(const WorldId& rhs) const { return _value <=> rhs._value; }
	constexpr bool operator==(const WorldId& rhs) const { return _value == rhs._value; }

private:
	uint64_t _value;
};

namespace std {
	template<>
	struct hash<WorldId> {
		size_t operator()(const WorldId& id) const noexcept
		{
			return std::hash<uint64_t>()(id.GetRaw());
		}
	};
}
