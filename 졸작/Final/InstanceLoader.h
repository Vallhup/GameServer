#pragma once

struct InstanceData
{
	XMFLOAT3 position;
	XMFLOAT3 rotation;
	XMFLOAT3 scale;
	bool distanceCull = false;
	float cullDistance = 60.0f;
	bool castShadow = true;
	bool twoSided = false;
	bool vertexAnim = false;
};

class InstanceLoader {
public:
	void Load(const wstring& fileName, const wstring& cullingFileName = L"");

	const unordered_map<string, vector<InstanceData>>& GetAllData() const { return instanceData; }

private:
	bool ContainsAny(const string& str, const vector<string>& keywords);
	bool ContainsAny(const string& str, initializer_list<string> keywords);

private:
	unordered_map<string, vector<InstanceData>> instanceData;
};