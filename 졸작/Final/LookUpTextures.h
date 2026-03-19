#pragma once

class LookUpTextures
{
public:
	void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

	wstring GetPathFromIndex(int idx) const { return allLUTs[idx].wstring(); }

private:
	void LoadAllLUTs(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);

private:
	vector<filesystem::path> allLUTs;
};

