#pragma once

struct InstanceData
{
	XMFLOAT3 position;
	XMFLOAT3 rotation;
	XMFLOAT3 scale;
};

class InstanceLoader {
public:
	void Load(const wstring& filename);

	const unordered_map<string, vector<InstanceData>>& GetAllData() const { return instanceData; }

private:
	unordered_map<string, vector<InstanceData>> instanceData;
};