#pragma once

#include "WorldDef.h"

#include <cstdint>
#include <string>
#include <vector>

class TerrainHeightRuntime final
{
public:
	TerrainHeightRuntime() = default;
	~TerrainHeightRuntime() = default;

	TerrainHeightRuntime(const TerrainHeightRuntime&) = delete;
	TerrainHeightRuntime& operator=(const TerrainHeightRuntime&) = delete;

	bool LoadFromFile(
		const std::string& rawFilePath,
		const TerrainHeightRawDef& def);

	void Unload();

	bool IsReady() const noexcept { return !_samples.empty(); }

	bool TrySampleHeight(
		float worldX,
		float worldZ,
		float& outHeight) const noexcept;

	const TerrainHeightRawDef& GetDef() const noexcept { return _def; }

private:
	uint16_t SampleAt(uint32_t x, uint32_t z) const noexcept;

private:
	TerrainHeightRawDef _def{};
	std::vector<uint16_t> _samples;
};
