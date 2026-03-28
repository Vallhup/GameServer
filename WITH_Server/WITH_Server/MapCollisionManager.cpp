#include "pch.h"
#include "MapCollisionManager.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void MapCollisionManager::LoadMapData(std::string_view path)
{
	int w, h, comp;
	if (stbi_uc* data = stbi_load(path.data(), &w, &h, &comp, 4))
	{
		if (w != Width || h != Height)
		{
			stbi_image_free(data);
			throw std::runtime_error(std::string("맵 충돌 데이터 크기 이상") + path.data());
		}

		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				const int i = (y * w + x) * 4;
				const uint8 r = data[i + 0];
				const uint8 g = data[i + 1];
				const uint8 b = data[i + 2];
				const uint8 a = data[i + 3];

				if (IsWhite(r, g, b))
					_mapGrid[y * w + x] = true;

				else if (IsBlack(r, g, b))
					_mapGrid[y * w + x] = false;

				else
				{
					stbi_image_free(data);
					throw std::runtime_error(std::string("맵 충돌 데이터 흑백 이상") + path.data());
				}
			}
		}

#ifdef _DEBUG
		std::cout << "Map Collision Data Load Success: " << path.data() << std::endl;
#endif

		stbi_image_free(data);
		return;
	}

	throw std::runtime_error(std::string("stbi_load failed: ") + stbi_failure_reason());
}

void MapCollisionManager::LoadHeightMap(std::string_view path)
{
	std::ifstream ifs(path.data(), std::ios::binary);
	if (!ifs.is_open())
	{
		throw std::runtime_error("Heightmap 데이터 로딩 실패");
		return;
	}

	uint64 fileSize = std::filesystem::file_size(path.data());
	uint64 pixelCount = fileSize / sizeof(uint16);
	uint32 dimension = static_cast<uint32>(std::sqrt(pixelCount));

	_heightMapWidth = dimension;
	_heightMapHeight = dimension;

	std::vector<uint16> rawData;
	rawData.resize(pixelCount);

	ifs.read(reinterpret_cast<char*>(rawData.data()), fileSize);
	ifs.close();

	_heightMapData.resize(pixelCount);
	for (uint64 i = 0; i < pixelCount; ++i)
	{
		_heightMapData[i] = rawData[i] / 65535.0f;
	}

#ifdef _DEBUG
	std::cout << "Heightmap loaded: " << _heightMapWidth << "x" << _heightMapHeight << std::endl;
#endif
}

bool MapCollisionManager::CanMove(float x, float z) const
{
	return true;
	auto [xIdx, zIdx] = WorldToGrid(x, z);
	if (xIdx < 0 || zIdx < 0 || xIdx * Width + xIdx >= Width * Height)
		return false;

	return _mapGrid[zIdx * Width + xIdx];
}

float MapCollisionManager::SampleHeightAt(float x, float z) const
{
	if (_heightMapData.empty()) return 0.0f;

	// TEMP : 맵 크기 160 고정
	float u = x / 160.0f;
	float v = z / 160.0f;

	if (u < 0.0f || u > 1.0f ||
		v < 0.0f || v > 1.0f) return 0.0f;

	float hx = u * (_heightMapWidth - 1);
	float hz = v * (_heightMapHeight - 1);

	int x0 = static_cast<int>(floor(hx));
	int z0 = static_cast<int>(floor(hz));
	int x1 = std::min<int>(x0 + 1, _heightMapWidth - 1);
	int z1 = std::min<int>(z0 + 1, _heightMapHeight - 1);

	float fx = hx - x0;
	float fz = hz - z0;

	float h00 = _heightMapData[z0 * _heightMapWidth + x0];
	float h10 = _heightMapData[z0 * _heightMapWidth + x1];
	float h01 = _heightMapData[z1 * _heightMapWidth + x0];
	float h11 = _heightMapData[z1 * _heightMapWidth + x1];

	float h0 = h00 * (1.0f - fx) + h10 * fx;
	float h1 = h01 * (1.0f - fx) + h11 * fx;
	float height = h0 * (1.0f - fz) + h1 * fz;
	
	// TEMP : Map Scale 600 고정
	return height * 600.0f;
}

std::pair<int, int> MapCollisionManager::WorldToGrid(float x, float z) const
{
	// TODO : World 좌표 정규화해서 Grid좌표로 바꾸는 코드
	static constexpr float mapSize = 160.0f;

	float u = x / mapSize;
	float v = z / mapSize;

	if (u < 0.0f || u > 1.0f ||
		v < 0.0f || v > 1.0f) return { -1, -1 };


	const float maxX = static_cast<float>(Width - 1);
	const float maxZ = static_cast<float>(Height - 1);

	int px = static_cast<int>(std::lround(u * maxX));
	int pz = static_cast<int>(std::lround(v * maxZ));

	px = std::clamp<int>(px, 0, maxX);
	pz = std::clamp<int>(pz, 0, maxZ);

	pz = maxZ - pz;

	return { px, pz };
}