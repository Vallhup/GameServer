#include "pch.h"
#include "InstanceLoader.h"

void InstanceLoader::Load(const wstring& fileName, const wstring& cullingFileName)
{
	ifstream cullingFile(cullingFileName);
	if (!cullingFile.is_open()) return;

	string name;
	vector<string> cullingDatas;

	while (cullingFile >> name)
	{
		cullingDatas.push_back(name);
	}

	ifstream file(fileName);
	if (!file.is_open()) return;

	string modelName;
	InstanceData data;

	while (file >> modelName
		>> data.position.x >> data.position.y >> data.position.z
		>> data.rotation.x >> data.rotation.y >> data.rotation.z
		>> data.scale.x >> data.scale.y >> data.scale.z)
	{
		data.distanceCull = false;
		data.castShadow = true;
		data.twoSided = false;

		if (ContainsAny(modelName, {"Grass"}))
		{
			data.distanceCull = true;
			data.cullDistance = 60.0f;
			data.castShadow = false;
			data.twoSided = true;
		}
		else if (ContainsAny(modelName, {"Tree", "SilverFir", "SM_EuropeanBeech_Dead_M_01", "SM_EuropeanBeech_Dead_XL_01",
			"SM_EuropeanBeech_L_01", "SM_EuropeanBeech_L_02", "SM_EuropeanBeech_L_03", "SM_EuropeanBeech_M_01", "SM_EuropeanBeech_S_01",
			"SM_EuropeanBeech_S_02", "SM_EuropeanBeech_XL_01", "SM_EuropeanBeech_XL_02", "SM_EuropeanBeech_XL_03", "SM_EuropeanBeech_XS_01",
			"SM_EuropeanBeech_XS_03", "Eagle", "WildCarrot", "Ivy"}))
		{
			data.distanceCull = false;
			data.twoSided = true;
		}
		else if (ContainsAny(modelName, cullingDatas))
		{
			data.distanceCull = true;
			data.cullDistance = 60.0f;
		}
		else if (ContainsAny(modelName, {"house"}))
		{
			data.distanceCull = true;
			data.cullDistance = 90.0f;
		}

		instanceData[modelName].push_back(data);
	}
}

bool InstanceLoader::ContainsAny(const string& str, const vector<string>& keywords)
{
	for (const auto& keyword : keywords)
	{
		if (str.find(keyword) != string::npos)
			return true;
	}

	return false;
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