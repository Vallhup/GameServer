#pragma once

class DX12Core;
class VertexIndexBuffer;

class Water
{
public:
	Water() = default;
	~Water() = default;

	void Initialize(DX12Core& core);
	void Render(ID3D12GraphicsCommandList* cmdList);

	D3D12_GPU_VIRTUAL_ADDRESS GetCBAddress() const { return objectCB->GetGPUVirtualAddress(); }

	void SetPosition(float x, float y, float z);
	void SetScale(float scl);

private:
	void BuildVertices();
	void BuildIndices();

private:
	shared_ptr<VertexIndexBuffer> vertexIndexBuffer;

	vector<Vertex> vertices;
	vector<UINT> indices;

	unique_ptr<UploadBuffer> objectCB;

	XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
	float scale = 1.0f;
};

