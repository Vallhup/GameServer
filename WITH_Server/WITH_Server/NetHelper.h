//#pragma once
//
//#include "SendBuffer.h"
//#include "types.h"
//#include "PacketFactory.h"
//
//#include "Protocol.pb.h"
//#include "NetId.h"
//#include "EntityType.h"
//
//namespace NetHelper {
//	inline SendBuffer* SCLoginPacket(NetId id)
//	{
//		Protocol::SC_LOGIN_PACKET login;
//		login.set_netid(id.GetRaw());
//
//		return PacketFactory::Serialize<Protocol::SC_LOGIN_PACKET>
//			(PacketType::SC_LOGIN, login);
//	}
//
//	inline SendBuffer* SCAddPacket(NetId id, EntityType type, float x, float y, float z, float yaw)
//	{
//		Protocol::SC_ADD_PACKET add;
//		add.set_netid(id.GetRaw());
//		add.set_typeid_(ToInt(type));
//		add.set_x(x);
//		add.set_y(y);
//		add.set_z(z);
//		add.set_yaw(yaw);
//
//		return PacketFactory::Serialize<Protocol::SC_ADD_PACKET>
//			(PacketType::SC_ADD, add);
//	}
//
//	inline SendBuffer* SCRemovePacket(NetId id)
//	{
//		Protocol::SC_REMOVE_PACKET remove;
//		remove.set_netid(id.GetRaw());
//
//		return PacketFactory::Serialize<Protocol::SC_REMOVE_PACKET>
//			(PacketType::SC_REMOVE, remove);
//	}
//
//	inline SendBuffer* SCMovePacket(NetId id, 
//		float x, float y, float z, float yaw)
//	{
//		Protocol::SC_MOVE_PACKET move;
//		move.set_netid(id.GetRaw());
//		move.set_x(x);
//		move.set_y(y);
//		move.set_z(z);
//		move.set_yaw(yaw);
//
//		return PacketFactory::Serialize<Protocol::SC_MOVE_PACKET>
//			(PacketType::SC_MOVE_OBJECT, move);
//	}
//
//	inline SendBuffer* SCAnimationChangePacket(NetId id, AnimationType curr)
//	{
//		Protocol::SC_ANIMATION_TRANSITION_PACKET anim;
//		anim.set_netid(id.GetRaw());
//		anim.set_curranim(ToInt(curr));
//
//		return PacketFactory::Serialize<Protocol::SC_ANIMATION_TRANSITION_PACKET>
//			(PacketType::SC_ANIMATION_CHANGE, anim);
//	}
//
//	inline SendBuffer* SCStatChangePacket(NetId id, int curHp,
//		int maxHp, int curStamina, int maxStamina, int power,
//		double aSpeed, int defense, double mSpeed)
//	{
//		Protocol::SC_STAT_CHANGE_PACKET stat;
//		stat.set_netid(id.GetRaw());
//		stat.set_curhp(curHp);
//		stat.set_maxhp(maxHp);
//		stat.set_curstamina(curStamina);
//		stat.set_maxstamina(maxStamina);
//		stat.set_power(power);
//		stat.set_attackspeed(aSpeed);
//		stat.set_defense(defense);
//		stat.set_movespeed(mSpeed);
//
//		return PacketFactory::Serialize<Protocol::SC_STAT_CHANGE_PACKET>
//			(PacketType::SC_STAT_CHANGE, stat);
//	}
//}