#pragma once

#include "SendBuffer.h"
#include "types.h"
#include "PacketFactory.h"

#include "Protocol.pb.h"

namespace NetHelper {
	inline SendBuffer* SCLoginPacket(uint32 id)
	{
		Protocol::SC_LOGIN_PACKET login;
		login.set_sessionid(id);

		return PacketFactory::Serialize<Protocol::SC_LOGIN_PACKET>
			(PacketType::SC_LOGIN, login);
	}

	inline SendBuffer* SCAddPacket(uint32 id, float x, float y, float z, float yaw)
	{
		Protocol::SC_ADD_PACKET add;
		add.set_sessionid(id);
		add.set_x(x);
		add.set_y(y);
		add.set_z(z);
		add.set_yaw(yaw);

		return PacketFactory::Serialize<Protocol::SC_ADD_PACKET>
			(PacketType::SC_ADD, add);
	}

	inline SendBuffer* SCRemovePacket(uint32 id)
	{
		Protocol::SC_REMOVE_PACKET remove;
		remove.set_ssessionid(id);

		return PacketFactory::Serialize<Protocol::SC_REMOVE_PACKET>
			(PacketType::SC_REMOVE, remove);
	}

	inline SendBuffer* SCMovePacket(uint32 id, 
		float x, float y, float z, float yaw)
	{
		Protocol::SC_MOVE_PACKET move;
		move.set_sessionid(id);
		move.set_x(x);
		move.set_y(y);
		move.set_z(z);
		move.set_yaw(yaw);

		return PacketFactory::Serialize<Protocol::SC_MOVE_PACKET>
			(PacketType::SC_MOVE_OBJECT, move);
	}

	inline SendBuffer* SCAnimationChangePacket(uint32 id,
		AnimationType prev, AnimationType curr)
	{
		Protocol::SC_ANIMATION_TRANSITION_PACKET anim;
		anim.set_prevanim(ToInt(prev));
		anim.set_curranim(ToInt(curr));

		return PacketFactory::Serialize<Protocol::SC_ANIMATION_TRANSITION_PACKET>
			(PacketType::SC_ANIMATION_CHANGE, anim);
	}
}