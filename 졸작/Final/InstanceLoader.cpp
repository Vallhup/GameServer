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
		if (ContainsAny(modelName, { "Anvil", "Apple", "Axe", "Bar", "Barrel", "Beam",
			"Bed", "Bench", "Bottle", "Bowl", "Candle", "Carrot", "Chair", "Crate", "Flagon",
			"Goblet", "Hammer", "Horse", "Jar", "KettlePot", "Log", "Mug", "Plate",
			"Pot", "Potato", "Pumpkin", "Sack", "Stone", "Stool", "Sword"}))
		{
			data.distanceCull = true;
			data.cullDistance = 50.0f;
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
