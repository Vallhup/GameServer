#pragma once

class DX12Core;

struct SparkParticle
{
	XMFLOAT3 position;
	XMFLOAT3 velocity;
	float age;
	float size;
};

struct SparkVertex
{
	XMFLOAT3 position;
	XMFLOAT2 uv;
	float alpha;
};

class ParrySparkEffect
{
public:
	void Initialize(ID3D12Device* device, UINT maxParticles = 64);
	void Update(float deltaTime, const XMFLOAT3& cameraPos);
	void Render(DX12Core& core);

	void Spawn(const XMFLOAT3& position, int count = 20);
	void Clear();

	void SetColor(const XMFLOAT4& color) { sparkColor = color; }
	void SetLifetime(float lifetime) { maxLifetime = lifetime; }
	void SetParticleSize(float size) { particleSize = size; }
	void SetSpeed(float speed) { sparkSpeed = speed; }
	void SetGravity(float g) { gravity = g; }
	void SetTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& path);

	bool IsAlive() const { return !particles.empty(); }

private:
	void BuildMesh(const XMFLOAT3& cameraPos);

private:
	vector<SparkParticle> particles;
	vector<SparkVertex> vertices;
	vector<UINT16> indices;

	unique_ptr<UploadBuffer> vertexBuffer;
	unique_ptr<UploadBuffer> indexBuffer;
	unique_ptr<UploadBuffer> sparkCB;

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	D3D12_INDEX_BUFFER_VIEW ibView = {};

	XMFLOAT4 sparkColor = { 1.0f, 0.7f, 0.3f, 1.0f };
	float maxLifetime = 0.3f;
	float particleSize = 0.02f;
	float sparkSpeed = 3.0f;
	float gravity = 9.8f;

	UINT maxParticles = 64;

	UINT textureIndex = 0xFFFFFFFF;
};
