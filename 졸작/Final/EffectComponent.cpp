#include "pch.h"
#include "EffectComponent.h"
#include "DX12Core.h"
#include "Shader.h"
#include "RootSignature.h"

void EffectComponent::Initialize(ID3D12Device* device, UINT maxElems)
{
	maxElements = maxElems;
	vertices.reserve(maxElements * 4);
	indices.reserve(maxElements * 6);

	vertexBuffer = make_unique<UploadBuffer>();
	vertexBuffer->Initialize(device, maxElements * 4 * sizeof(EffectVertex));

	indexBuffer = make_unique<UploadBuffer>();
	indexBuffer->Initialize(device, maxElements * 6 * sizeof(UINT16));

	constantBuffer = make_unique<UploadBuffer>();
	constantBuffer->Initialize(device, sizeof(EffectConstants));
}

void EffectComponent::Render(DX12Core& core, const XMFLOAT3& cameraPos)
{
	BuildMesh(cameraPos);

	if (vertices.empty() || indices.empty())
		return;

	vertexBuffer->CopyData(vertices.data(), vertices.size() * sizeof(EffectVertex));
	vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vbView.SizeInBytes = static_cast<UINT>(vertices.size() * sizeof(EffectVertex));
	vbView.StrideInBytes = sizeof(EffectVertex);

	indexBuffer->CopyData(indices.data(), indices.size() * sizeof(UINT16));
	ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	ibView.SizeInBytes = static_cast<UINT>(indices.size() * sizeof(UINT16));
	ibView.Format = DXGI_FORMAT_R16_UINT;

	EffectConstants constants;
	constants.color = effectColor;
	constants.textureIndex = textureIndex;
	constantBuffer->CopyData(&constants, sizeof(EffectConstants));

	auto cmdList = core.GetGraphicsCmdList();
	cmdList->SetPipelineState(core.GetShader()->GetPSO(GetPSOType()));
	cmdList->SetGraphicsRootSignature(core.GetRootSig()->Get());
	cmdList->SetGraphicsRootConstantBufferView(0, core.GetFrameCB()->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(23, constantBuffer->GetGPUVirtualAddress());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &vbView);
	cmdList->IASetIndexBuffer(&ibView);
	cmdList->DrawIndexedInstanced(static_cast<UINT>(indices.size()), 1, 0, 0, 0);
}
