#pragma once

struct CollisionBox 
{
	float minX, maxX, minZ, maxZ;

	CollisionBox(float min_x, float max_x, float min_z, float max_z) : minX(min_x), maxX(max_x), minZ(min_z), maxZ(max_z) {}
};

class CollisionManager
{
	DECLARE_SINGLE(CollisionManager);

public:
	void Init();

	bool IsInsideCollisionBox(float x, float z);
	bool IsInsidePVPBox(float x, float z);

private:
	std::vector<CollisionBox> collisionBoxes;
	CollisionBox pvpBox = { -20.2914f, 20.2322f, -19.9385f, 20.0144f };
	bool initialized = { false };

	//x- 왼쪽
	//x+ 오른쪽
	//z- 위
	//z+ 아래
};

