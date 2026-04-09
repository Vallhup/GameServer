#pragma once
#include "EffectComponent.h"

struct DustParticle
{
	XMFLOAT3 position;
	XMFLOAT3 velocity;
	float age;
	float size;
};

class FootDustComponent : public EffectComponent
{
public:
	void Update(float deltaTime) override;

	void Spawn(const XMFLOAT3& position, int count = 6);
	void Clear();

	void SetParticleSize(float size) { particleSize = size; }

protected:
	PSOType GetPSOType() const override;
	void BuildMesh(const XMFLOAT3& cameraPos) override;

private:
	vector<DustParticle> particles;
	float particleSize = 0.01f;
	bool rightFootSpawned = false;
	bool leftFootSpawned = false;
};
