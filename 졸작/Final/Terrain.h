#pragma once

class DX12Core;
class VertexIndexBuffer;
class Material;

class Terrain
{
public:
	Terrain() = default;
	~Terrain() = default;

	void Initialize(DX12Core& core, const wstring& basePath, const wstring& heightmapPath, int gridSize, float worldSize, float heightScale, float tileSize);

	void Render(ID3D12GraphicsCommandList* cmdList);

	float SampleHeightAt(float worldX, float worldZ) const;

	VertexIndexBuffer* GetVertexIndexBuffer() const { return vertexIndexBuffer.get(); }
	Material* GetMaterial() const { return material.get(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetCBAddress() const { return objectCB->GetGPUVirtualAddress(); }

	void SetPosition(float x, float y, float z);
	void SetRotation(float x, float y, float z);
	void SetScale(float x, float y, float z);

private:
	void LoadHeightmap(const wstring& path);
	void BuildVertices(float tileSize);
	void BuildIndices();

private:
	shared_ptr<VertexIndexBuffer> vertexIndexBuffer;
	shared_ptr<Material> material;

	vector<float> heightmapData;
	int heightmapWidth = 0;
	int heightmapHeight = 0;

	int gridSize = 0;
	float worldSize = 0.0f;
	float heightScale = 0.0f;

	vector<Vertex> vertices;
	vector<UINT> indices;

	unique_ptr<UploadBuffer> objectCB;

	XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 rotation = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
};
