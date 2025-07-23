#include "pch.h"
#include "CollisionManager.h"

void CollisionManager::Init()
{ 
	if (initialized)
	{
		cout << "이미 충돌체들 저장됨!" << '\n';
		return;
	}

	std::ifstream in{ "CollisionData/CollisionData.txt" };
	if (!in)
	{
		cout << "충돌체 파일 열 수 없음!" << '\n';
		return;
	}
	std::string line;
	collisionBoxes.clear();

	while (std::getline(in, line)) {
		istringstream iss(line);
		float min_x, max_x, min_z, max_z;

		if (iss >> min_x >> max_x >> min_z >> max_z) {
			collisionBoxes.emplace_back(min_x, max_x, min_z, max_z);
		}
	}
	
	in.close();
	initialized = true;	
	
	//cout << "충돌체 저장 완료!" << '\n';
}

bool CollisionManager::IsInsideCollisionBox(float x, float z)
{
	if (!initialized)
	{
		cout << "충돌체 초기화 아직 안됨!!" << '\n';
		return false;
	}

	for (const auto& box : collisionBoxes)
	{
		if (x >= box.minX && x <= box.maxX &&
			z >= box.minZ && z <= box.maxZ)
			return true;
	}

	return false;
}
