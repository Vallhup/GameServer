#include "pch.h"
#include "Sector.h"

std::pair<int, int> Sector::getSector(int x, int y)
{
	return { x / SECTOR_SIZE, y / SECTOR_SIZE };
}

std::pair<std::pair<int, int>, std::pair<int, int>> Sector::getSectorRange(int x, int y)
{
	auto compute = [](int coord) -> std::pair<int, int>
		{
			int min = std::clamp((coord - VIEW_RANGE) / SECTOR_SIZE, 0, SECTOR_COUNT - 1);
			int max = std::clamp((coord + VIEW_RANGE) / SECTOR_SIZE, 0, SECTOR_COUNT - 1);

			return { min, max };
		};

	std::pair<int, int> xRange = compute(x);
	std::pair<int, int> yRange = compute(y);

	return { xRange, yRange };
}
