#pragma once

class DX12Core;

struct TrailVertex
{
	XMFLOAT3 position;
	XMFLOAT2 uv;
	float alpha;
};

struct TrailPoint
{
	XMFLOAT3 top;		// 칼 끝
	XMFLOAT3 bottom;	// 칼 손잡이
	float age;			// 생성 후 경과 시간
};

class TrailRenderer
{
public:
	void Initialize(ID3D12Device* device, UINT maxPoints = 32);
	void Update(float deltaTime);
	void Render(DX12Core& core);

	void AddPoint(const XMFLOAT3& top, const XMFLOAT3& bottom);
	void SetActive(bool active);
	void Clear();

	void SetColor(const XMFLOAT4& color) { trailColor = color; }
	void SetLifetime(float lifetime) { maxLifetime = lifetime; }
	void SetWidth(float width) { trailWidth = width; }

	bool IsActive() const { return isActive; }

private:
	void BuildMesh();

private:
	vector<TrailPoint> points;
	vector<TrailVertex> vertices;
	vector<UINT16> indices;

	unique_ptr<UploadBuffer> vertexBuffer;
	unique_ptr<UploadBuffer> indexBuffer;
	unique_ptr<UploadBuffer> trailCB;

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	D3D12_INDEX_BUFFER_VIEW ibView = {};

	XMFLOAT4 trailColor = { 1.0f, 0.8f, 0.4f, 1.0f };	// 주황빛 색상
	float maxLifetime = 0.15f;		// 트레일 지속 시간
	float trailWidth = 1.0f;		// 트레일 너비 스케일

	UINT maxPoints = 32;
	bool isActive = false;
	bool isDirty = false;
};
