#pragma once

#include "types.h"

class NetId {
	static constexpr int ID_BITS{ 32 };
	static constexpr int GEN_BITS{ 16 };
	static constexpr int SPARE_BITS{ 16 };

	static_assert(ID_BITS + GEN_BITS + SPARE_BITS == 64);

	static constexpr int ID_SHIFT{ 0 };
	static constexpr int GEN_SHIFT{ ID_SHIFT + ID_BITS }; // 0 + 32 = 32
	static constexpr int SPARE_SHIFT{ GEN_SHIFT + GEN_BITS }; // 32 + 16 = 48

	static constexpr uint64 ID_FIELD_MASK{ (uint64(1) << ID_BITS) - 1u };
	static constexpr uint64 GEN_FIELD_MASK{ (uint64(1) << GEN_BITS) - 1u };
	static constexpr uint64 SPARE_FIELD_MASK{ (uint64(1) << SPARE_BITS) - 1u };

	static constexpr uint64 ID_MASK{ ID_FIELD_MASK << ID_SHIFT };
	static constexpr uint64 GEN_MASK{ GEN_FIELD_MASK << GEN_SHIFT };
	static constexpr uint64 SPARE_MASK{ SPARE_FIELD_MASK << SPARE_SHIFT };

public:
	constexpr NetId() : _value(0) {}
	explicit constexpr NetId(uint64 v) : _value(v) {}

	static constexpr NetId Invalid() { return NetId(0); }
	constexpr bool IsValid() const { return _value != 0; }

	static constexpr NetId Create(uint32 id, uint16 gen, uint16 spare = 0)
	{
		assert(id != 0);
		return NetId{
			(static_cast<uint64>(id) << ID_SHIFT) |
			(static_cast<uint64>(gen) << GEN_SHIFT) |
			(static_cast<uint64>(spare) << SPARE_SHIFT)
		};
	}

	constexpr uint32 GetId() const
	{
		return static_cast<uint32>((_value & ID_MASK) >> ID_SHIFT);
	}

	constexpr uint16 GetGen() const
	{
		return static_cast<uint16>((_value & GEN_MASK) >> GEN_SHIFT);
	}

	constexpr uint16 GetSpare() const
	{
		return static_cast<uint16>((_value & SPARE_MASK) >> SPARE_SHIFT);
	}

	constexpr uint64 ToUInt64() const { return _value; }

	constexpr bool operator==(const NetId& other) const { return _value == other._value; }
	constexpr bool operator!=(const NetId& other) const { return _value != other._value; }

private:
	uint64 _value;
};