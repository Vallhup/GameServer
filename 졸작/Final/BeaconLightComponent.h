#pragma once
#include "EffectComponent.h"

class BeaconLightComponent : public EffectComponent
{
public:
	void Update(float deltaTime) override;

	void Spawn(const XMFLOAT3& position);
	void Stop() { alive = false; vertices.clear(); indices.clear(); }

	void SetPosition(const XMFLOAT3& position) { beaconPos = position; }
	void SetSize(float size) { beaconSize = size; }
	void SetTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const wstring& path);

	bool IsAlive() const { return alive; }

protected:
	PSOType GetPSOType() const override;
	void BuildMesh(const XMFLOAT3& cameraPos) override;

private:
	XMFLOAT3 beaconPos = {};
	float age = 0.0f;
	float beaconSize = 1.0f;
	bool alive = false;

	static constexpr float PULSE_SPEED = 2.0f;
	static constexpr float PULSE_BASE = 0.85f;
	static constexpr float PULSE_DEPTH = 0.15f;
};
