#include "pch.h"
#include "SkyBox.h"
#include "VertexIndexBuffer.h"
#include "Material.h"
#include "DX12Core.h"
#include "Shader.h"
#include "RootSignature.h"

void SkyBox::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	InitializeMesh(device, cmdList);
	InitializeSkyBoxCB(device);

	skyboxCubeMapIndex = Material::RegisterCubeMap(device, cmdList, L"../Assets/Skybox/skybox7.dds");

	Material::RegisterCubeMap(device, cmdList, L"../Assets/Skybox/skybox7_irradiance.dds");
	Material::RegisterCubeMap(device, cmdList, L"../Assets/Skybox/skybox7_radiance.dds");

	Material::RegisterTexture(device, cmdList, L"../Assets/Skybox/brdf_lut.png");
}

void SkyBox::RenderSkyBox(DX12Core& core, ID3D12GraphicsCommandList* cmdList)
{
	if (!skyboxMesh || skyboxCubeMapIndex == 0xFFFFFFFF) return;

	cmdList->SetPipelineState(core.GetShader()->GetPSO(PSOType::Skybox));
	cmdList->SetGraphicsRootSignature(core.GetRootSig()->Get());
	cmdList->SetGraphicsRootConstantBufferView(0, core.GetFrameCB()->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(20, skyboxCB->GetGPUVirtualAddress());

	Material::BindBindlessResources(cmdList);

	skyboxMesh->Bind(cmdList);
	skyboxMesh->Draw(cmdList);
}

void SkyBox::InitializeMesh(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	float s = 1.0f;

	vector<Vertex> vertices = {
		{{-s, -s, -s}}, {{-s,  s, -s}}, {{ s,  s, -s}}, {{ s, -s, -s}},
		{{ s, -s,  s}}, {{ s,  s,  s}}, {{-s,  s,  s}}, {{-s, -s,  s}},
		{{-s,  s, -s}}, {{-s,  s,  s}}, {{ s,  s,  s}}, {{ s,  s, -s}},
		{{-s, -s,  s}}, {{-s, -s, -s}}, {{ s, -s, -s}}, {{ s, -s,  s}},
		{{-s, -s,  s}}, {{-s,  s,  s}}, {{-s,  s, -s}}, {{-s, -s, -s}},
		{{ s, -s, -s}}, {{ s,  s, -s}}, {{ s,  s,  s}}, {{ s, -s,  s}}
	};

	vector<UINT> indices = {
		0,1,2, 0,2,3,
		4,5,6, 4,6,7,
		8,9,10, 8,10,11,
		12,13,14, 12,14,15,
		16,17,18, 16,18,19,
		20,21,22, 20,22,23
	};

	skyboxMesh = make_shared<VertexIndexBuffer>();
	skyboxMesh->Initialize(device, cmdList, vertices, indices);
}

void SkyBox::InitializeSkyBoxCB(ID3D12Device* device)
{
	skyboxCB = make_unique<UploadBuffer>();
	skyboxCB->Initialize(device, sizeof(SkyboxConstants));

	skyboxData.skyTintColor = { 1.0f, 1.0f, 1.0f };
	skyboxData.skyExposure = 1.0f;
	skyboxData.skySaturation = 1.0f;

	skyboxCB->CopyData(&skyboxData, sizeof(SkyboxConstants));
}

void SkyBox::UpdateConstants()
{
	skyboxCB->CopyData(&skyboxData, sizeof(SkyboxConstants));
}

