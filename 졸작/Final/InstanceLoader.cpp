#include "pch.h"
#include "InstanceLoader.h"

void InstanceLoader::Load(const wstring& filename)
{
	ifstream file(filename);
	if (!file.is_open()) return;

	string modelName;
	InstanceData data;

	while (file >> modelName
		>> data.position.x >> data.position.y >> data.position.z
		>> data.rotation.x >> data.rotation.y >> data.rotation.z
		>> data.scale.x >> data.scale.y >> data.scale.z)
	{
		instanceData[modelName].push_back(data);
	}
}
