#pragma once

class DX12Core;

struct FlameParticle
{
	XMFLOAT3 position;
	float age;
	float size;
};

struct FlameVertex
{
	XMFLOAT3 position;
	XMFLOAT2 uv;
	float alpha;
};

class FlameEffect
{
public:
	void Initialize(ID3D12Device* device, UINT maxParticles = 32);
	void Update(float deltaTime, const XMFLOAT3& cameraPos);
	void Render(DX12Core& core);

	void Spawn(const XMFLOAT3& position, int count = 1);
	void Clear();

	void SetColor(const XMFLOAT4& color) { flameColor = color; }
	void SetFadeInTime(float time) { fadeInTime = time; }
	void SetParticleSize(float size) { particleSize = size; }
	void SetTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& path);

	private:
	void BuildMesh(const XMFLOAT3& cameraPos);

	private:
	vector<FlameParticle> particles;
	vector<FlameVertex> vertices;
	vector<UINT16> indices;

	unique_ptr<UploadBuffer> vertexBuffer;
	unique_ptr<UploadBuffer> indexBuffer;
	unique_ptr<UploadBuffer> flameCB;

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	D3D12_INDEX_BUFFER_VIEW ibView = {};

	XMFLOAT4 flameColor = { 1.0f, 0.55f, 0.1f, 1.0f };  // 주황-노란 불꽃색
	float fadeInTime = 1.0f; 
	float particleSize = 0.5f;

	UINT maxParticles = 32;

	UINT textureIndex = 0xFFFFFFFF;
};
