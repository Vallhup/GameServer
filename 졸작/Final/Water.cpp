#include "pch.h"
#include "Water.h"
#include "DX12Core.h"
#include "VertexIndexBuffer.h"

void Water::Initialize(DX12Core& core)
{
	BuildVertices();
	BuildIndices();

	vertexIndexBuffer = make_shared<VertexIndexBuffer>();
	vertexIndexBuffer->Initialize(
		core.GetDevice(),
		core.GetGraphicsCmdList(),
		vertices,
		indices
	);

	objectCB = make_unique<UploadBuffer>();
	objectCB->Initialize(core.GetDevice(), CONSTANT_BUFFER_ALIGNMENT);

	ObjectConstants obj = {};
	obj.world = XMMatrixTranspose(XMMatrixIdentity());
	obj.useTexture = 0;
	obj.useInstancing = 0;
	obj.materialIndex = 0;

	objectCB->CopyData(&obj, sizeof(ObjectConstants), 0);
}

void Water::Render(ID3D12GraphicsCommandList* cmdList)
{
	if (vertexIndexBuffer)
	{
		vertexIndexBuffer->Bind(cmdList);
		vertexIndexBuffer->Draw(cmdList);
	}
}

void Water::SetPosition(float x, float y, float z)
{
	position = { x, y, z };

	XMMATRIX scaleMat = XMMatrixScaling(scale, 1.0f, scale);
	XMMATRIX transMat = XMMatrixTranslation(x, y, z);
	XMMATRIX world = XMMatrixTranspose(scaleMat * transMat);

	ObjectConstants obj = {};
	obj.world = world;
	obj.useTexture = 0;
	obj.useInstancing = 0;
	obj.materialIndex = 0;

	objectCB->CopyData(&obj, sizeof(ObjectConstants), 0);
}

void Water::SetScale(float scl)
{
	scale = scl;

	XMMATRIX scaleMat = XMMatrixScaling(scale, 1.0f, scale);
	XMMATRIX transMat = XMMatrixTranslation(position.x, position.y, position.z);
	XMMATRIX world = XMMatrixTranspose(scaleMat * transMat);

	ObjectConstants obj = {};
	obj.world = world;
	obj.useTexture = 0;
	obj.useInstancing = 0;
	obj.materialIndex = 0;

	objectCB->CopyData(&obj, sizeof(ObjectConstants), 0);
}

void Water::BuildVertices()
{
	vertices.clear();
	vertices.reserve(4);

	Vertex v0 = {}, v1 = {}, v2 = {}, v3 = {};
	
	v0.pos = XMFLOAT3(1.0f, 0.0f, 1.0f);
	v1.pos = XMFLOAT3(1.0f, 0.0f, -1.0f);
	v2.pos = XMFLOAT3(-1.0f, 0.0f, -1.0f);
	v3.pos = XMFLOAT3(-1.0f, 0.0f, 1.0f);

	v0.uv = XMFLOAT2(1.0f, 0.0f);  // 우상
	v1.uv = XMFLOAT2(1.0f, 1.0f);  // 우하
	v2.uv = XMFLOAT2(0.0f, 1.0f);  // 좌하
	v3.uv = XMFLOAT2(0.0f, 0.0f);  // 좌상

	v0.normal = v1.normal = v2.normal = v3.normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
	v0.tangent = v1.tangent = v2.tangent = v3.tangent = XMFLOAT3(1.0f, 0.0f, 0.0f);
	v0.color = v1.color = v2.color = v3.color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

	vertices.push_back(v0);
	vertices.push_back(v1);
	vertices.push_back(v2);
	vertices.push_back(v3);
}

void Water::BuildIndices()
{
	// 0 1 2 & 0 2 3
	indices.clear();
	indices.reserve(6);

	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);

	indices.push_back(0);
	indices.push_back(2);
	indices.push_back(3);
}
