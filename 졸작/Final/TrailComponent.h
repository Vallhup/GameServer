#pragma once
#include "EffectComponent.h"

struct TrailPoint
{
	XMFLOAT3 top;
	XMFLOAT3 bottom;
	float age;
};

// 검 본 하나가 남기는 트레일 한 줄기. 쌍검 캐릭터는 본마다 한 줄기씩 둔다.
struct TrailStrip
{
	int boneIndex = 45;
	vector<TrailPoint> points;
};

class TrailComponent : public EffectComponent
{
public:
	void Update(float deltaTime) override;

	// 검 본 인덱스(스켈레톤마다 다름). 쌍검이면 두 검의 본을 모두 전달
	void SetBoneIndices(const vector<int>& indices);
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

	vector<TrailStrip> trails{ TrailStrip{} };  // 기본 1줄기(본 45)
	float trailWidth = 1.0f;
	float bladeLength = 1.0f;  // 검 본 기준 칼날 길이 — 무기마다 다름
	bool isActive = false;
};
