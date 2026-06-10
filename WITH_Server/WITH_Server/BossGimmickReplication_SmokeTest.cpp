#include "pch.h"

#include <cassert>
#include <cmath>
#include <iostream>

#include "BossGimmickReplicationProtocol.h"
#include "ECS/System/Phase0_AI/BossGimmickAnimationPolicy.h"

namespace
{
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
		event.brokenByPlayerNetId = 4004;

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
		assert(packet.brokenbyplayernetid() == 4004);
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

	void RunBossGimmickAnimationPolicyTest()
	{
		BossGimmickStateComp phaseTransition{};
		phaseTransition.activeType = BossGimmickType::PhaseTransitionObjects;
		phaseTransition.stage = BossGimmickStage::Telegraph;

		assert(BossGimmickAnimationPolicy::ShouldUseEntryAnimation(
			phaseTransition));
		assert(BossGimmickAnimationPolicy::EntryAnimationFor(
			phaseTransition.activeType) == AnimationId::FinalBoss_50Percent);
		assert(std::fabs(
			BossGimmickAnimationPolicy::EntryAnimationDurationFor(
				phaseTransition.activeType) - 13.4f) < 0.001f);

		BossGimmickStateComp finalSafeZone{};
		finalSafeZone.activeType = BossGimmickType::FinalSafeZone;
		finalSafeZone.stage = BossGimmickStage::Telegraph;

		assert(BossGimmickAnimationPolicy::ShouldUseEntryAnimation(
			finalSafeZone));
		assert(BossGimmickAnimationPolicy::EntryAnimationFor(
			finalSafeZone.activeType) == AnimationId::FinalBoss_0Percent);
		assert(std::fabs(
			BossGimmickAnimationPolicy::EntryAnimationDurationFor(
				finalSafeZone.activeType) - 12.0f) < 0.001f);

		finalSafeZone.stage = BossGimmickStage::Active;
		assert(!BossGimmickAnimationPolicy::ShouldUseEntryAnimation(
			finalSafeZone));
	}
}

void RunBossGimmickReplicationSmokeTests()
{
	RunBossGimmickObjectPacketFillTest();
	RunBossGimmickZonePacketFillTest();
	RunBossGimmickAnimationPolicyTest();

	std::cout << "[PASS] BossGimmickReplication smoke\n";
}
