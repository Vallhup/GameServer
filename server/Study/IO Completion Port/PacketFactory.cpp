#include "pch.h"
#include "PacketFactory.h"

std::vector<char> PacketFactory::BuildLoginPacket(const GameObject& target)
{
	SC_LOGIN_INFO_PACKET login;
	login.id = target.GetId();
	login.size = sizeof(login);
	login.type = SC_LOGIN_INFO;
	login.x = target.GetX();
	login.y = target.GetY();

	return Serialize(login);
}

std::vector<char> PacketFactory::BuildAddPacket(const GameObject& target)
{
	SC_ADD_OBJECT_PACKET add;
	add.id = target.GetId();
	add.size = sizeof(add);
	add.type = SC_ADD_OBJECT;
	add.x = target.GetX();
	add.y = target.GetY();
	strcpy_s(add.name, NAME_SIZE, target.GetName().c_str());
	add.name[NAME_SIZE - 1] = '\0';

	return Serialize(add);
}

std::vector<char> PacketFactory::BuildMovePacket(const GameObject& target)
{
	SC_MOVE_OBJECT_PACKET move;
	move.id = target.GetId();
	move.size = sizeof(move);
	move.type = SC_MOVE_OBJECT;
	move.x = target.GetX();
	move.y = target.GetY();
	move.move_time = target._lastMoveTime;

	return Serialize(move);
}

std::vector<char> PacketFactory::BuildRemovePacket(const GameObject& target)
{
	SC_REMOVE_OBJECT_PACKET remove;
	remove.id = target.GetId();
	remove.size = sizeof(remove);
	remove.type = SC_REMOVE_OBJECT;

	return Serialize(remove);
}

std::vector<char> PacketFactory::BuildStatChangePacket(const GameObject& target)
{
	SC_STAT_CHANGE_PACKET stat;
	stat.exp = target.GetExp();
	stat.hp = target.GetHp();
	stat.level = target.GetLevel();
	stat.max_hp = target.GetMaxHp();
	stat.size = sizeof(stat);
	stat.type = SC_STAT_CHANGE;

	return Serialize(stat);
}

std::vector<char> PacketFactory::BuildPartyRequestPacket(int fromId)
{
	SC_PARTY_REQUEST_PACKET request;
	request.fromId = fromId;
	request.size = sizeof(request);
	request.type = SC_PARTY_REQUEST;

	return Serialize(request);
}

std::vector<char> PacketFactory::BuildPartyResultPacket(int targetId, bool accept)
{
	SC_PARTY_RESULT_PACKET result;
	result.acceptFlag = accept;
	result.size = sizeof(result);
	result.targetId = targetId;
	result.type = SC_PARTY_RESULT;

	return Serialize(result);
}

std::vector<char> PacketFactory::BuildPartyUpdatePacket(const Party& party)
{
	auto infos = party.GetMemberInfos();

	SC_PARTY_UPDATE_PACKET update;
	update.memberCount = infos.size();
	update.partyId = party.GetId();
	update.size = sizeof(update) + sizeof(MemberInfo) * infos.size();
	update.type = SC_PARTY_UPDATE;

	std::vector<char> buf(update.size);
	std::memcpy(buf.data(), &update, sizeof(update));
	std::memcpy(buf.data() + sizeof(update), infos.data(), sizeof(MemberInfo) * infos.size());

	return buf;
}

std::vector<char> PacketFactory::BuildPartyDisbandPacket(const Party& party)
{
	SC_PARTY_DISBAND_PACKET disband;
	disband.partyId = party.GetId();
	disband.size = sizeof(disband);
	disband.type = SC_PARTY_DISBAND;

	return Serialize(disband);
}

std::vector<char> PacketFactory::BuildAddItemPacket(char itemId, int count)
{
	SC_ADD_ITEM_PACKET item;
	item.count = count;
	item.itemId = itemId;
	item.size = sizeof(item);
	item.type = SC_ADD_ITEM;

	return Serialize(item);
}

std::vector<char> PacketFactory::BuildUseItemOkPacket(char itemId)
{
	SC_USE_ITEM_OK_PACKET itemOk;
	itemOk.itemId = itemId;
	itemOk.size = sizeof(itemOk);
	itemOk.type = SC_USE_ITEM_OK;

	return Serialize(itemOk);
}
