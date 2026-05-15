#pragma once
#include "EffectComponent.h"

struct BloodInstance
{
	XMFLOAT3 position;
	XMFLOAT3 velocity;
	float age;        
	float size;
	float rotation;   
	UINT texSlot;     
};

class BloodImpactComponent : public EffectComponent
{
public:
	void InitializeBlood(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, UINT maxElems = 32);

	void Update(float deltaTime) override;
	void Spawn(const XMFLOAT3& impactPos, const XMFLOAT3& impactDir);
	void Clear();

	void SetBaseSpeed(float s) { spawnSpeed = s; }
	void SetBaseSize(float s) { baseSize = s; }
	void SetGravity(float g) { gravity = g; }
	void SetDragHalfLife(float t) { dragHalfLife = t; }
	void SetCountPerSlot(int n) { countPerSlot = n; }
	void SetStretch(float s) { stretchFactor = s; }

	bool IsAlive() const { return !instances.empty(); }

protected:
	PSOType GetPSOType() const override;
	void BuildMesh(const XMFLOAT3& cameraPos) override;
	void Render(DX12Core& core, const XMFLOAT3& cameraPos) override;

private:
	vector<BloodInstance> instances;
	UINT texIndices[3] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };

	UINT slotIndexStart[3] = { 0, 0, 0 };
	UINT slotIndexCount[3] = { 0, 0, 0 };

	unique_ptr<UploadBuffer> slotCBuffers[3];

	float spawnSpeed = 4.0f;
	float baseSize = 0.6f;
	float gravity = 2.5f;
	float dragHalfLife = 0.15f;
	int countPerSlot = 4;
	float stretchFactor = 5.5f;

	static constexpr int FLIPBOOK_COLS = 8;
	static constexpr int FLIPBOOK_ROWS = 4;
	static constexpr int FLIPBOOK_FRAMES = FLIPBOOK_COLS * FLIPBOOK_ROWS;
};
