#pragma once
#include "EffectComponent.h"

struct FlameParticle
{
	XMFLOAT3 position;
	float age;
	float size;
};

class FlameComponent : public EffectComponent
{
public:
	void Update(float deltaTime) override;

	void Spawn(const XMFLOAT3& position, int count = 1);
	void Clear();

	void SetFadeInTime(float time) { fadeInTime = time; }
	void SetParticleSize(float size) { particleSize = size; }
	void SetTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& path);

protected:
	PSOType GetPSOType() const override;
	void BuildMesh(const XMFLOAT3& cameraPos) override;

private:
	vector<FlameParticle> particles;
	float fadeInTime = 1.0f;
	float particleSize = 0.5f;
};
