#pragma once

class DX12Core;

struct DustParticle
{
	XMFLOAT3 position;
	XMFLOAT3 velocity;
	float age;
	float size;
};

struct DustVertex
{
	XMFLOAT3 position;
	XMFLOAT2 uv;
	float alpha;
};

class FootDustEffect
{
public:
	void Initialize(ID3D12Device* device, UINT maxParticles = 32);
	void Update(float deltaTime, const XMFLOAT3& cameraPos);
	void Render(DX12Core& core);

	void Spawn(const XMFLOAT3& position, int count = 6);
	void Clear();

	void SetColor(const XMFLOAT4& color) { dustColor = color; }
	void SetLifetime(float lifetime) { maxLifetime = lifetime; }
	void SetParticleSize(float size) { particleSize = size; }

private:
	void BuildMesh(const XMFLOAT3& cameraPos);

private:
	vector<DustParticle> particles;
	vector<DustVertex> vertices;
	vector<UINT16> indices;

	unique_ptr<UploadBuffer> vertexBuffer;
	unique_ptr<UploadBuffer> indexBuffer;
	unique_ptr<UploadBuffer> dustCB;

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	D3D12_INDEX_BUFFER_VIEW ibView = {};

	XMFLOAT4 dustColor = { 0.6f, 0.5f, 0.4f, 0.7f };  // 흙먼지 색상
	float maxLifetime = 0.4f;
	float particleSize = 0.01f;

	UINT maxParticles = 32;
	bool isDirty = false;
};
