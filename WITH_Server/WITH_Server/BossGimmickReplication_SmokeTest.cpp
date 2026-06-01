#include "pch.h"

#include <cassert>
#include <cmath>
#include <iostream>

#include "BossGimmickReplicationProtocol.h"

namespace
{
	void RunBossGimmickStatePacketFillTest()
	{
		FrameworkRuntime::FrameResult::BossGimmickStateEvent event{};
		event.bossNetId = NetId{ 1001 };
		event.gimmickSeq = 7;
		event.gimmickType =
			static_cast<uint32_t>(
				Protocol::BOSS_GIMMICK_TYPE_PHASE_TRANSITION_OBJECTS);
		event.stage =
			static_cast<uint32_t>(Protocol::BOSS_GIMMICK_STAGE_ACTIVE);
		event.durationSec = 8.0f;
		event.remainingSec = 3.5f;

		Protocol::SC_BOSS_GIMMICK_STATE_PACKET packet;
		BossGimmickReplicationProtocol::FillStatePacket(packet, event);

		assert(packet.bossnetid() == 1001);
		assert(packet.gimmickseq() == 7);
		assert(packet.gimmicktype() ==
			Protocol::BOSS_GIMMICK_TYPE_PHASE_TRANSITION_OBJECTS);
		assert(packet.stage() == Protocol::BOSS_GIMMICK_STAGE_ACTIVE);
		assert(std::fabs(packet.durationsec() - 8.0f) < 0.001f);
		assert(std::fabs(packet.remainingsec() - 3.5f) < 0.001f);
	}

	void RunBossGimmickObjectPacketFillTest()
	{
		FrameworkRuntime::FrameResult::BossGimmickObjectSyncEvent event{};
		event.bossNetId = NetId{ 1001 };
		event.gimmickSeq = 7;
		event.objectNetId = 2002;
		event.state =
			static_cast<uint32_t>(Protocol::BOSS_GIMMICK_OBJECT_STATE_BROKEN);
		event.x = 1.0f;
		event.y = 2.0f;
		event.z = 3.0f;
		event.radius = 0.75f;
		event.curHp = 0;
		event.maxHp = 60;

		Protocol::SC_BOSS_GIMMICK_OBJECT_SYNC_PACKET packet;
		BossGimmickReplicationProtocol::FillObjectSyncPacket(packet, event);

		assert(packet.bossnetid() == 1001);
		assert(packet.gimmickseq() == 7);
		assert(packet.objectnetid() == 2002);
		assert(packet.state() == Protocol::BOSS_GIMMICK_OBJECT_STATE_BROKEN);
		assert(std::fabs(packet.x() - 1.0f) < 0.001f);
		assert(std::fabs(packet.y() - 2.0f) < 0.001f);
		assert(std::fabs(packet.z() - 3.0f) < 0.001f);
		assert(std::fabs(packet.radius() - 0.75f) < 0.001f);
		assert(packet.curhp() == 0);
		assert(packet.maxhp() == 60);
	}

	void RunBossGimmickZonePacketFillTest()
	{
		FrameworkRuntime::FrameResult::BossGimmickZoneSyncEvent event{};
		event.bossNetId = NetId{ 1001 };
		event.gimmickSeq = 8;
		event.zoneNetId = 3003;
		event.state =
			static_cast<uint32_t>(
				Protocol::BOSS_GIMMICK_OBJECT_STATE_DESPAWNED);
		event.x = 4.0f;
		event.y = 5.0f;
		event.z = 6.0f;
		event.radius = 1.25f;

		Protocol::SC_BOSS_GIMMICK_ZONE_SYNC_PACKET packet;
		BossGimmickReplicationProtocol::FillZoneSyncPacket(packet, event);

		assert(packet.bossnetid() == 1001);
		assert(packet.gimmickseq() == 8);
		assert(packet.zonenetid() == 3003);
		assert(packet.state() ==
			Protocol::BOSS_GIMMICK_OBJECT_STATE_DESPAWNED);
		assert(std::fabs(packet.x() - 4.0f) < 0.001f);
		assert(std::fabs(packet.y() - 5.0f) < 0.001f);
		assert(std::fabs(packet.z() - 6.0f) < 0.001f);
		assert(std::fabs(packet.radius() - 1.25f) < 0.001f);
	}
}

void RunBossGimmickReplicationSmokeTests()
{
	RunBossGimmickStatePacketFillTest();
	RunBossGimmickObjectPacketFillTest();
	RunBossGimmickZonePacketFillTest();

	std::cout << "[PASS] BossGimmickReplication smoke\n";
}
