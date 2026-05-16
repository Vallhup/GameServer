#pragma once
#include "Component.h"

class DX12Core;
enum class PSOType;

struct EffectVertex
{
	XMFLOAT3 position;
	XMFLOAT2 uv;
	float alpha;
};

struct EffectConstants
{
	XMFLOAT4 color;
	UINT textureIndex;
	XMFLOAT3 padding;
};

class EffectComponent : public Component
{
public:
	virtual ~EffectComponent() = default;

	void Initialize(ID3D12Device* device, UINT maxElements);
	virtual void Render(DX12Core& core, const XMFLOAT3& cameraPos);

	void SetColor(const XMFLOAT4& color) { effectColor = color; }
	void SetLifetime(float lifetime) { maxLifetime = lifetime; }

protected:
	virtual PSOType GetPSOType() const = 0;
	virtual void BuildMesh(const XMFLOAT3& cameraPos) = 0;

protected:
	unique_ptr<UploadBuffer> vertexBuffer;
	unique_ptr<UploadBuffer> indexBuffer;
	unique_ptr<UploadBuffer> constantBuffer;

	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	D3D12_INDEX_BUFFER_VIEW ibView = {};

	vector<EffectVertex> vertices;
	vector<UINT16> indices;

	XMFLOAT4 effectColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	float maxLifetime = 1.0f;
	UINT maxElements = 32;
	UINT textureIndex = 0xFFFFFFFF;
};
