#include "pch.h"
#include "LookUpTextures.h"
#include "Material.h"

void LookUpTextures::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	LoadAllLUTs(device, cmdList);
}

void LookUpTextures::LoadAllLUTs(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	wstring rootPath = L"../Assets/LUTs";

	for (const auto& entry : filesystem::recursive_directory_iterator(rootPath))
	{
		if (entry.is_regular_file() && entry.path().extension() == L".png")
			allLUTs.push_back(entry.path());
	}

	sort(allLUTs.begin(), allLUTs.end());

	for (const auto& lut : allLUTs)
		Material::RegisterLUT(device, cmdList, lut.wstring());
}
