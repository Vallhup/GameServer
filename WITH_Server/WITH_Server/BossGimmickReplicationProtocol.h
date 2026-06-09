#pragma once

#include "FrameworkRuntime.h"
#include "Protocol.pb.h"

namespace BossGimmickReplicationProtocol
{
	inline Protocol::BossGimmickObjectState ToProtocolObjectState(
		uint32_t value) noexcept
	{
		switch (value)
		{
		case Protocol::BOSS_GIMMICK_OBJECT_STATE_UPDATED:
			return Protocol::BOSS_GIMMICK_OBJECT_STATE_UPDATED;
		case Protocol::BOSS_GIMMICK_OBJECT_STATE_BROKEN:
			return Protocol::BOSS_GIMMICK_OBJECT_STATE_BROKEN;
		case Protocol::BOSS_GIMMICK_OBJECT_STATE_DESPAWNED:
			return Protocol::BOSS_GIMMICK_OBJECT_STATE_DESPAWNED;
		case Protocol::BOSS_GIMMICK_OBJECT_STATE_SPAWNED:
		default:
			return Protocol::BOSS_GIMMICK_OBJECT_STATE_SPAWNED;
		}
	}

	inline void FillObjectSyncPacket(
		Protocol::SC_BOSS_GIMMICK_OBJECT_SYNC_PACKET& packet,
		const FrameworkRuntime::FrameResult::BossGimmickObjectSyncEvent& event)
	{
		packet.set_bossnetid(event.bossNetId.GetRaw());
		packet.set_gimmickseq(event.gimmickSeq);
		packet.set_objectnetid(event.objectNetId);
		packet.set_state(ToProtocolObjectState(event.state));
		packet.set_x(event.x);
		packet.set_y(event.y);
		packet.set_z(event.z);
		packet.set_radius(event.radius);
		packet.set_curhp(event.curHp);
		packet.set_maxhp(event.maxHp);
		packet.set_brokenbyplayernetid(event.brokenByPlayerNetId);
	}

	inline void FillZoneSyncPacket(
		Protocol::SC_BOSS_GIMMICK_ZONE_SYNC_PACKET& packet,
		const FrameworkRuntime::FrameResult::BossGimmickZoneSyncEvent& event)
	{
		packet.set_bossnetid(event.bossNetId.GetRaw());
		packet.set_gimmickseq(event.gimmickSeq);
		packet.set_zonenetid(event.zoneNetId);
		packet.set_state(ToProtocolObjectState(event.state));
		packet.set_x(event.x);
		packet.set_y(event.y);
		packet.set_z(event.z);
		packet.set_radius(event.radius);
	}
}
