#pragma once
#include "EffectComponent.h"

class ParryStreakComponent : public EffectComponent
{
public:
	void Update(float deltaTime) override;

	void Spawn(const XMFLOAT3& position);

	void SetWidth(float w) { streakWidth = w; }
	void SetHeight(float h) { streakHeight = h; }
	void SetTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& path);

	bool IsAlive() const { return alive; }

protected:
	PSOType GetPSOType() const override;
	void BuildMesh(const XMFLOAT3& cameraPos) override;

private:
	XMFLOAT3 streakPos = {};
	float age = 0.0f;
	float streakWidth = 1.5f;
	float streakHeight = 0.3f;
	bool alive = false;
};
