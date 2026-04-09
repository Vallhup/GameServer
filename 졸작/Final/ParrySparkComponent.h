#pragma once
#include "EffectComponent.h"

struct SparkParticle
{
	XMFLOAT3 position;
	XMFLOAT3 velocity;
	float age;
	float size;
};

class ParrySparkComponent : public EffectComponent
{
public:
	void Update(float deltaTime) override;

	void Spawn(const XMFLOAT3& position, int count = 20);
	void Clear();

	void SetParticleSize(float size) { particleSize = size; }
	void SetSpeed(float speed) { sparkSpeed = speed; }
	void SetGravity(float g) { gravity = g; }
	void SetTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& path);

	bool IsAlive() const { return !particles.empty(); }

protected:
	PSOType GetPSOType() const override;
	void BuildMesh(const XMFLOAT3& cameraPos) override;

private:
	vector<SparkParticle> particles;
	float particleSize = 0.02f;
	float sparkSpeed = 3.0f;
	float gravity = 9.8f;
};
