#pragma once

class DX12Core;
class VertexIndexBuffer;

struct WaterConstants {
	XMFLOAT4 waterColor;
	float waterTime;
	float waveSpeed;
	float waveStrength;
	float padding;
};

class Water
{
public:
	Water() = default;
	~Water() = default;

	void Initialize(DX12Core& core);
	void Update(float deltaTime);
	void Render(DX12Core& core, ID3D12GraphicsCommandList* cmdList);

	D3D12_GPU_VIRTUAL_ADDRESS GetCBAddress() const { return objectCB->GetGPUVirtualAddress(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetWaterCBAddress() const { return waterCB->GetGPUVirtualAddress(); }

	void SetPosition(float x, float y, float z);
	void SetScale(float x, float y, float z);
	
	void SetColor(const XMFLOAT4& color) { waterColor = color; }
	void SetWaveSpeed(float speed) { waveSpeed = speed; }
	void SetWaveStrength(float strength) { waveStrength = strength; }

private:
	void BuildVertices();
	void BuildIndices();
	void UpdateWaterCB();

private:
	shared_ptr<VertexIndexBuffer> vertexIndexBuffer;

	vector<Vertex> vertices;
	vector<UINT> indices;

	unique_ptr<UploadBuffer> objectCB;
	unique_ptr<UploadBuffer> waterCB;

	XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };

	XMFLOAT4 waterColor = { 0.0f, 0.5f, 0.75f, 0.8f };
	float totalTime = 0.0f;
	float waveSpeed = 0.05f;
	float waveStrength = 0.02f;
};
