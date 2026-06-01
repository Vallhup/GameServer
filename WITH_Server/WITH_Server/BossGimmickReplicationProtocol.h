#pragma once

#include "FrameworkRuntime.h"
#include "Protocol.pb.h"

namespace BossGimmickReplicationProtocol
{
	inline Protocol::BossGimmickType ToProtocolGimmickType(
		uint32_t value) noexcept
	{
		switch (value)
		{
		case Protocol::BOSS_GIMMICK_TYPE_PHASE_TRANSITION_OBJECTS:
			return Protocol::BOSS_GIMMICK_TYPE_PHASE_TRANSITION_OBJECTS;
		case Protocol::BOSS_GIMMICK_TYPE_FINAL_SAFE_ZONE:
			return Protocol::BOSS_GIMMICK_TYPE_FINAL_SAFE_ZONE;
		default:
			return Protocol::BOSS_GIMMICK_TYPE_NONE;
		}
	}

	inline Protocol::BossGimmickStage ToProtocolGimmickStage(
		uint32_t value) noexcept
	{
		switch (value)
		{
		case Protocol::BOSS_GIMMICK_STAGE_TELEGRAPH:
			return Protocol::BOSS_GIMMICK_STAGE_TELEGRAPH;
		case Protocol::BOSS_GIMMICK_STAGE_ACTIVE:
			return Protocol::BOSS_GIMMICK_STAGE_ACTIVE;
		case Protocol::BOSS_GIMMICK_STAGE_RESOLVE:
			return Protocol::BOSS_GIMMICK_STAGE_RESOLVE;
		case Protocol::BOSS_GIMMICK_STAGE_COMPLETED:
			return Protocol::BOSS_GIMMICK_STAGE_COMPLETED;
		case Protocol::BOSS_GIMMICK_STAGE_CANCELLED:
			return Protocol::BOSS_GIMMICK_STAGE_CANCELLED;
		default:
			return Protocol::BOSS_GIMMICK_STAGE_NONE;
		}
	}

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

	inline void FillStatePacket(
		Protocol::SC_BOSS_GIMMICK_STATE_PACKET& packet,
		const FrameworkRuntime::FrameResult::BossGimmickStateEvent& event)
	{
		packet.set_bossnetid(event.bossNetId.GetRaw());
		packet.set_gimmickseq(event.gimmickSeq);
		packet.set_gimmicktype(ToProtocolGimmickType(event.gimmickType));
		packet.set_stage(ToProtocolGimmickStage(event.stage));
		packet.set_durationsec(event.durationSec);
		packet.set_remainingsec(event.remainingSec);
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
