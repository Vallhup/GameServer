#pragma once

struct InstanceData
{
	XMFLOAT3 position;
	XMFLOAT3 rotation;
	XMFLOAT3 scale;
	bool distanceCull = false;
	float cullDistance = 50.0;
};

class InstanceLoader {
public:
	void Load(const wstring& filename);

	const unordered_map<string, vector<InstanceData>>& GetAllData() const { return instanceData; }

private:
	bool ContainsAny(const string& str, initializer_list<string> keywords);

private:
	unordered_map<string, vector<InstanceData>> instanceData;
};