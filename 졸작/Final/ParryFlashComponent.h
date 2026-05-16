#pragma once
#include "EffectComponent.h"

class ParryFlashComponent : public EffectComponent
{
public:
	void Update(float deltaTime) override;

	void Spawn(const XMFLOAT3& position);

	void SetSize(float size) { flashSize = size; }
	void SetTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& path);

	bool IsAlive() const { return alive; }

protected:
	PSOType GetPSOType() const override;
	void BuildMesh(const XMFLOAT3& cameraPos) override;

private:
	XMFLOAT3 flashPos = {};
	float age = 0.0f;
	float flashSize = 0.5f;
	bool alive = false;
};
