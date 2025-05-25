#pragma once

constexpr int MAP_SIZE = 2000;
constexpr int SECTOR_SIZE = 20;
constexpr int SECTOR_COUNT = MAP_SIZE / SECTOR_SIZE;

class Sector
{
public:
	// 해당 Sector에 Client add/remove
	void addObject(int id)
	{
		std::unique_lock lock{ _mutex };
		_objects.insert(id);
	}

	void removeObject(int id)
	{
		std::unique_lock lock{ _mutex };
		_objects.erase(id);
	}

	// 섹터 내 모든 client ID를 out에 복사
	void collectObject(std::unordered_set<int>& out) const
	{
		std::shared_lock lock{ _mutex };
		out.insert(_objects.begin(), _objects.end());
	}

public:
	static std::pair<int, int> getSector(int x, int y);
	static std::pair<std::pair<int, int>, std::pair<int, int>> getSectorRange(int x, int y);

private:
	std::unordered_set<int> _objects;
	mutable std::shared_mutex _mutex;
};

