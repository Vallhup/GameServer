#pragma once

class VertexIndexBuffer;
class DX12Core;

class SkyBox
{
public:
	void Initialize(ID3D12Device * device, ID3D12GraphicsCommandList * cmdList);
	void RenderSkyBox(DX12Core& core, ID3D12GraphicsCommandList* cmdList);

private:
	void InitializeMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void RegisterCubeMap(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& ddsPath);

private:
	shared_ptr<VertexIndexBuffer> skyboxMesh;
	UINT skyboxCubeMapIndex = 0xFFFFFFFF;
};

