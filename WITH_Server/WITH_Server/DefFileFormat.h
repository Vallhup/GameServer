#pragma once

#include <cstdint>

constexpr uint32_t MakeDefFileMagic(char a, char b, char c, char d) noexcept
{
	return
		static_cast<uint32_t>(static_cast<uint8_t>(a)) |
		(static_cast<uint32_t>(static_cast<uint8_t>(b)) << 8) |
		(static_cast<uint32_t>(static_cast<uint8_t>(c)) << 16) |
		(static_cast<uint32_t>(static_cast<uint8_t>(d)) << 24);
}

constexpr uint32_t kDefFileMagic = MakeDefFileMagic('W', 'D', 'E', 'F');

enum class DefDataEncoding : uint32_t
{
	Binary = 1,
	Json = 2
};

enum class DefTypeTag : uint32_t
{
	Unknown = 0,
	Animation = 1,
	Action = 2,
	Character = 3,
	Buff = 4,
	SpawnSet = 5
};

struct DefFileHeader
{
	uint32_t magic = kDefFileMagic;
	DefDataEncoding encoding = DefDataEncoding::Binary;
	DefTypeTag typeTag = DefTypeTag::Unknown;
	uint16_t versionMajor = 1;
	uint16_t versionMinor = 0;
	uint32_t entryCount = 0;
	uint32_t payloadSizeBytes = 0;
};

constexpr DefFileHeader MakeDefFileHeader(
	DefTypeTag typeTag,
	DefDataEncoding encoding,
	uint32_t entryCount,
	uint32_t payloadSizeBytes,
	uint16_t versionMajor = 1,
	uint16_t versionMinor = 0) noexcept
{
	return DefFileHeader
	{
		.magic = kDefFileMagic,
		.encoding = encoding,
		.typeTag = typeTag,
		.versionMajor = versionMajor,
		.versionMinor = versionMinor,
		.entryCount = entryCount,
		.payloadSizeBytes = payloadSizeBytes
	};
}

constexpr bool IsValidDefFileHeader(const DefFileHeader& header) noexcept
{
	return header.magic == kDefFileMagic &&
		header.typeTag != DefTypeTag::Unknown;
}
