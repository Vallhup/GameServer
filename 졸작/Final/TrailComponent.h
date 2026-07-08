#pragma once
#include "EffectComponent.h"

struct TrailPoint
{
	XMFLOAT3 top;
	XMFLOAT3 bottom;
	float age;
};

struct TrailStrip
{
	int boneIndex = 45;
	vector<TrailPoint> points;
};

class TrailComponent : public EffectComponent
{
public:
	void Update(float deltaTime) override;

	void SetBoneIndices(const vector<int>& indices);
	void SetActivationClips(const vector<string>& clips) { activationClips = clips; }
	void SetActive(bool active);
	void Clear();

	void SetWidth(float width) { trailWidth = width; }
	void SetBladeLength(float length) { bladeLength = length; }

	bool IsActive() const { return isActive; }

protected:
	PSOType GetPSOType() const override;
	void BuildMesh(const XMFLOAT3& cameraPos) override;

private:
	void AddPoint(TrailStrip& strip, const XMFLOAT3& top, const XMFLOAT3& bottom);
	bool HasPoints() const;

	vector<TrailStrip> trails{ TrailStrip{} };  
	vector<string> activationClips{ "AttackCombo1", "AttackCombo2", "AttackCombo3" };
	float trailWidth = 1.0f;
	float bladeLength = 1.0f;  
	bool isActive = false;
};
