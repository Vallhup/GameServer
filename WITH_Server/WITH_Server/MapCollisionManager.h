#pragma once

static inline bool IsWhite(uint8 r, uint8 g, uint8 b)
{
	return (r == 255 && g == 255 && b == 255);
}

static inline bool IsBlack(uint8 r, uint8 g, uint8 b)
{
	return (r == 0 && g == 0 && b == 0);
}

class MapCollisionManager {
	static constexpr int Width{ 1025 };
	static constexpr int Height{ 1025 };

public:
	static MapCollisionManager& Get()
	{
		static MapCollisionManager instance;
		return instance;
	}

	void LoadMapData(std::string_view path);
	void LoadHeightMap(std::string_view path);

	bool CanMove(float x, float z) const;
	float SampleHeightAt(float x, float z) const;

private:
	std::pair<int, int> WorldToGrid(float x, float z) const;

	// TEMP : 맵 여러개 되면 확장 필요
	std::array<bool, Width * Height> _mapGrid{ false, };

	uint32 _heightMapWidth;
	uint32 _heightMapHeight;
	std::vector<float> _heightMapData;
};

