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
		if (ContainsAny(modelName, { "Grass", "Bush" }))
		{
			data.distanceCull = true;
			data.cullDistance = 35.0f;
		}
		else
			data.distanceCull = false;

		instanceData[modelName].push_back(data);
	}
}

bool InstanceLoader::ContainsAny(const string& str, initializer_list<string> keywords)
{
	for (const auto& keyword : keywords)
	{
		if (str.find(keyword) != string::npos)
			return true;
	}

	return false;
}
