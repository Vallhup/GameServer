#pragma once
#include "GameObject.h"		

namespace Protocol { class SC_BOSS_GIMMICK_OBJECT_SYNC_PACKET; }

class GimmickDiamond : public GameObject
{
public:
	void Init(DX12Core& core, const XMFLOAT4& color);
	void SyncFrom(const Protocol::SC_BOSS_GIMMICK_OBJECT_SYNC_PACKET& packet);
	void Update(float deltaTime) override;

	int GetObjectNetId() const { return objectNetId; }

	bool ShouldRemove() const;

private:
	void BuildDiamondMesh(DX12Core& core, const XMFLOAT4& color);

	static constexpr float SHAKE_DURATION = 0.5f;
	static constexpr float SHAKE_MAG = 0.15f;

	static constexpr float RING_SHADE = 0.25f;
	static constexpr float APEX_SHADE = 0.9f;
	static constexpr float APEX_MIN_BRIGHT = 0.08f;

	int bossNetId = -1;
	int objectNetId = -1;
	uint32_t gimmickSeq = 0;
	uint32_t curHp = 0;
	uint32_t maxHp = 0;
	uint32_t prevHp = 0;
	int state = 0;
	bool markedForRemoval = false;	

	XMFLOAT3 syncedPos{ 0.0f, 0.0f, 0.0f };
	float spin = 0.0f;
	float shakeTimer = 0.0f;
};
