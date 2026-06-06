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

private:
	void BuildDiamondMesh(DX12Core& core, const XMFLOAT4& color);

	int bossNetId = -1;
	int objectNetId = -1;
	uint32_t gimmickSeq = 0;
	uint32_t curHp = 0;
	uint32_t maxHp = 0;
	int state = 0;

	float spin = 0.0f;	
};
