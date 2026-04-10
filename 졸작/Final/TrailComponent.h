#pragma once
#include "EffectComponent.h"

struct TrailPoint
{
	XMFLOAT3 top;
	XMFLOAT3 bottom;
	float age;
};

class TrailComponent : public EffectComponent
{
public:
	void Update(float deltaTime) override;

	void AddPoint(const XMFLOAT3& top, const XMFLOAT3& bottom);
	void SetActive(bool active);
	void Clear();

	void SetWidth(float width) { trailWidth = width; }

	bool IsActive() const { return isActive; }

protected:
	PSOType GetPSOType() const override;
	void BuildMesh(const XMFLOAT3& cameraPos) override;

private:
	vector<TrailPoint> points;
	float trailWidth = 1.0f;
	bool isActive = false;
};
