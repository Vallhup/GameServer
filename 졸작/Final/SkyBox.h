#pragma once

class VertexIndexBuffer;
class DX12Core;

struct SkyboxConstants
{
	XMFLOAT3 skyTintColor;
	float skyExposure;
	float skySaturation;
	XMFLOAT3 skyPadding;
};

struct SkySun
{
	XMFLOAT3 direction = { -0.73f, -1.39f, -1.0f };
	XMFLOAT3 color = { 1.0f, 1.0f, 1.0f };
	float intensity = 0.0f;
};

class SkyBox
{
public:
	void Initialize(ID3D12Device * device, ID3D12GraphicsCommandList * cmdList);
	void RenderSkyBox(DX12Core& core, ID3D12GraphicsCommandList* cmdList);
	void UpdateConstants();

	SkyboxConstants& GetConstants() { return skyboxData; }
	SkySun& GetSun() { return sun; }
	const SkySun& GetSun() const { return sun; }

private:
	void InitializeMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
	void InitializeSkyBoxCB(ID3D12Device* device);

private:
	shared_ptr<VertexIndexBuffer> skyboxMesh;
	UINT skyboxCubeMapIndex = 0xFFFFFFFF;

	unique_ptr<UploadBuffer> skyboxCB;
	SkyboxConstants skyboxData = {};

	SkySun sun;
};

