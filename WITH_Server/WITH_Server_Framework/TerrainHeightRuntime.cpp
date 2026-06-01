#include "pch.h"
#include "TerrainHeightRuntime.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <utility>

namespace
{
	bool IsValidDef(const TerrainHeightRawDef& def) noexcept
	{
		return
			def.width >= 2 &&
			def.height >= 2 &&
			std::isfinite(def.cellSizeX) &&
			std::isfinite(def.cellSizeZ) &&
			def.cellSizeX > 0.0f &&
			def.cellSizeZ > 0.0f &&
			std::isfinite(def.rotationYDegrees) &&
			std::isfinite(def.heightScale) &&
			std::isfinite(def.heightOffset) &&
			def.sampleFormat == TerrainHeightSampleFormat::UInt16LE;
	}

	float DegreesToRadians(float degrees) noexcept
	{
		constexpr float kPi = 3.14159265358979323846f;
		return degrees * (kPi / 180.0f);
	}

	bool TryGetExpectedByteCount(
		const TerrainHeightRawDef& def,
		uintmax_t& outExpectedBytes) noexcept
	{
		const uintmax_t width = static_cast<uintmax_t>(def.width);
		const uintmax_t height = static_cast<uintmax_t>(def.height);

		if (width != 0 && height > std::numeric_limits<uintmax_t>::max() / width)
			return false;

		const uintmax_t sampleCount = width * height;
		if (sampleCount > std::numeric_limits<uintmax_t>::max() / sizeof(uint16_t))
			return false;

		outExpectedBytes = sampleCount * sizeof(uint16_t);
		return true;
	}

	bool TryGetSampleCount(
		const TerrainHeightRawDef& def,
		size_t maxSampleCount,
		size_t& outSampleCount) noexcept
	{
		const uintmax_t sampleCount =
			static_cast<uintmax_t>(def.width) *
			static_cast<uintmax_t>(def.height);

		if (sampleCount > std::numeric_limits<size_t>::max())
			return false;

		if (sampleCount > static_cast<uintmax_t>(maxSampleCount))
			return false;

		outSampleCount = static_cast<size_t>(sampleCount);
		return true;
	}
}

void TerrainHeightRuntime::Unload()
{
	_def = TerrainHeightRawDef{};
	_samples.clear();
	_samples.shrink_to_fit();
}

bool TerrainHeightRuntime::LoadFromFile(
	const std::string& rawFilePath,
	const TerrainHeightRawDef& def)
{
	if (!IsValidDef(def))
		return false;

	uintmax_t expectedBytes = 0;
	if (!TryGetExpectedByteCount(def, expectedBytes))
		return false;

	std::error_code ec;
	const uintmax_t actualBytes = std::filesystem::file_size(rawFilePath, ec);
	if (ec || actualBytes != expectedBytes)
		return false;

	std::ifstream ifs(rawFilePath, std::ios::binary);
	if (!ifs.is_open())
		return false;

	std::vector<uint16_t> samples;

	size_t sampleCount{ 0 };
	if (!TryGetSampleCount(def, samples.max_size(), sampleCount))
		return false;

	samples.resize(sampleCount);

	for (uint16_t& sample : samples)
	{
		unsigned char bytes[2]{};
		ifs.read(reinterpret_cast<char*>(bytes), sizeof(bytes));
		if (ifs.fail())
			return false;

		sample =
			static_cast<uint16_t>(bytes[0]) |
			static_cast<uint16_t>(static_cast<uint16_t>(bytes[1]) << 8);
	}

	TerrainHeightRawDef loadedDef = def;
	if (loadedDef.path.empty())
		loadedDef.path = rawFilePath;

	_def = std::move(loadedDef);
	_samples = std::move(samples);
	return true;
}

bool TerrainHeightRuntime::TrySampleHeight(
	float worldX,
	float worldZ,
	float& outHeight) const noexcept
{
	if (!IsReady() || !std::isfinite(worldX) || !std::isfinite(worldZ))
		return false;

	const float deltaX = worldX - _def.originX;
	const float deltaZ = worldZ - _def.originZ;
	const float rotationRadians = DegreesToRadians(_def.rotationYDegrees);
	const float cosY = std::cos(rotationRadians);
	const float sinY = std::sin(rotationRadians);

	const float localX = deltaX * cosY - deltaZ * sinY;
	const float localZ = deltaX * sinY + deltaZ * cosY;

	// Equivalent to the client formula after inverse-transforming world X/Z
	// into Unity Terrain local space: hx = localX / cellSize.
	const float gridX = localX / _def.cellSizeX;
	const float logicalGridZ = localZ / _def.cellSizeZ;
	const float gridZ = _def.flipZ
		? static_cast<float>(_def.height - 1u) - logicalGridZ
		: logicalGridZ;

	const float maxGridX = static_cast<float>(_def.width - 1u);
	const float maxGridZ = static_cast<float>(_def.height - 1u);

	if (gridX < 0.0f || gridX > maxGridX || gridZ < 0.0f || gridZ > maxGridZ)
		return false;

	const uint32_t x0 = static_cast<uint32_t>(std::floor(gridX));
	const uint32_t z0 = static_cast<uint32_t>(std::floor(gridZ));
	const uint32_t x1 = (x0 + 1u < _def.width) ? x0 + 1u : x0;
	const uint32_t z1 = (z0 + 1u < _def.height) ? z0 + 1u : z0;

	const float tx = gridX - static_cast<float>(x0);
	const float tz = gridZ - static_cast<float>(z0);

	const float h00 = static_cast<float>(SampleAt(x0, z0));
	const float h10 = static_cast<float>(SampleAt(x1, z0));
	const float h01 = static_cast<float>(SampleAt(x0, z1));
	const float h11 = static_cast<float>(SampleAt(x1, z1));

	const float h0 = h00 + (h10 - h00) * tx;
	const float h1 = h01 + (h11 - h01) * tx;
	const float sample = h0 + (h1 - h0) * tz;

	outHeight = sample * _def.heightScale + _def.heightOffset;
	return true;
}

uint16_t TerrainHeightRuntime::SampleAt(uint32_t x, uint32_t z) const noexcept
{
	return _samples[static_cast<size_t>(z) * _def.width + x];
}
