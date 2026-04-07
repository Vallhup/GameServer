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

	waterCB = make_unique<UploadBuffer>();
	waterCB->Initialize(core.GetDevice(), sizeof(WaterConstants));

	SetPosition(0, 0, 0);
	UpdateWaterCB();
}

void Water::Update(float deltaTime)
{
	totalTime += deltaTime;
	UpdateWaterCB();
}

void Water::UpdateWaterCB()
{
	WaterConstants wt = {};
	wt.waterColor = waterColor;
	wt.waterTime = totalTime;
	wt.waveSpeed = waveSpeed;
	wt.waveStrength = waveStrength;

	waterCB->CopyData(&wt, sizeof(WaterConstants), 0);
}

void Water::Render(DX12Core& core, ID3D12GraphicsCommandList* cmdList)
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

	XMMATRIX scaleMat = XMMatrixScaling(scale.x, scale.y, scale.z);
	XMMATRIX transMat = XMMatrixTranslation(x, y, z);
	XMMATRIX world = XMMatrixTranspose(scaleMat * transMat);

	ObjectConstants obj = {};
	obj.world = world;
	obj.useTexture = 2; // 2 means Water
	obj.useInstancing = 0;
	obj.materialIndex = 0;

	objectCB->CopyData(&obj, sizeof(ObjectConstants), 0);
}

void Water::SetScale(float x, float y, float z)
{
	scale = { x, y, z };
	SetPosition(position.x, position.y, position.z);
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

	v0.uv = XMFLOAT2(1.0f, 0.0f);
	v1.uv = XMFLOAT2(1.0f, 1.0f);
	v2.uv = XMFLOAT2(0.0f, 1.0f);
	v3.uv = XMFLOAT2(0.0f, 0.0f);

	v0.normal = v1.normal = v2.normal = v3.normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
	v0.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	vertices.push_back(v0);
	vertices.push_back(v1);
	vertices.push_back(v2);
	vertices.push_back(v3);
}

void Water::BuildIndices()
{
	indices = { 0, 1, 2, 0, 2, 3 };
}
