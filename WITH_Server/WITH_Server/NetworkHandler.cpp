#include "pch.h"
#include "NetworkHandler.h"
#include "Framework.h"

#include "PacketFactory.h"

NetworkHandler::NetworkHandler()
{
	_handlerTable[(uint16)PacketType::CS_LOGIN] = &NetworkHandler::HandleConnect;
	_handlerTable[(uint16)PacketType::CS_MOVE] = &NetworkHandler::HandleMove;
	_handlerTable[(uint16)PacketType::CS_ATTACK] = &NetworkHandler::HandleAttack;
	_handlerTable[(uint16)PacketType::CS_DODGE] = &NetworkHandler::HandleDodge;
	_handlerTable[(uint16)PacketType::CS_GUARD] = &NetworkHandler::HandleGuard;
	_handlerTable[(uint16)PacketType::CS_PARRY] = &NetworkHandler::HandleParry;
}

bool NetworkHandler::HandleConnect(uint32 id, const PacketHeader& header, const BYTE* data)
{
	Protocol::CS_LOGIN_PACKET login;
	if (not PacketFactory::Deserialize(header, data, &login))
		return false;

	ConnectEvent cn{ id };
	Event ev{ EventType::EV_CONNECT, cn };
	Framework::Get().eventQueue.push(ev);

	return true;
}

bool NetworkHandler::HandleMove(uint32 id, const PacketHeader& header, const BYTE* data)
{
	Protocol::CS_MOVE_PACKET move;
	if (not PacketFactory::Deserialize(header, data, &move))
		return false;

	NetId nId = Framework::Get().listener.GetIdMap().GetPlayer(id);
	MoveEvent mv{ nId, move.inputx(), move.inputz(), move.yaw(), move.isrun() };
	Event ev{ EventType::EV_MOVE, mv };
	Framework::Get().eventQueue.push(ev);

	return true;
}

bool NetworkHandler::HandleAttack(uint32 id, const PacketHeader& header, const BYTE* data)
{
	Protocol::CS_ATTACK_PACKET attack;
	if (not PacketFactory::Deserialize(header, data, &attack))
		return false;

	NetId nId = Framework::Get().listener.GetIdMap().GetPlayer(id);
	ActionEvent ac{ nId, ActionRequestType::Attack, attack.dirx(), attack.dirz(), true };
	Event ev{ EventType::EV_ACTION, ac };
	Framework::Get().eventQueue.push(ev);

	return true;
}

bool NetworkHandler::HandleDodge(uint32 id, const PacketHeader& header, const BYTE* data)
{
	Protocol::CS_DODGE_PACKET dodge;
	if (not PacketFactory::Deserialize(header, data, &dodge))
		return false;

	NetId nId = Framework::Get().listener.GetIdMap().GetPlayer(id);
	ActionEvent ac{ nId, ActionRequestType::Dodge, dodge.dirx(), dodge.dirz(), true };
	Event ev{ EventType::EV_ACTION, ac };
	Framework::Get().eventQueue.push(ev);

	return true;
}

bool NetworkHandler::HandleGuard(uint32 id, const PacketHeader& header, const BYTE* data)
{
	Protocol::CS_GUARD_PACKET guard;
	if (not PacketFactory::Deserialize(header, data, &guard))
		return false;

	NetId nId = Framework::Get().listener.GetIdMap().GetPlayer(id);
	ActionEvent ac{ nId, ActionRequestType::Guard, 0.0f, 0.0f, guard.input() };
	Event ev{ EventType::EV_ACTION, ac };
	Framework::Get().eventQueue.push(ev);

	return true;
}

bool NetworkHandler::HandleParry(uint32 id, const PacketHeader& header, const BYTE* data)
{
	Protocol::CS_PARRY_PACKET parry;
	if (not PacketFactory::Deserialize(header, data, &parry))
		return false;

	NetId nId = Framework::Get().listener.GetIdMap().GetPlayer(id);
	ActionEvent ac{ nId, ActionRequestType::Parry, parry.dirx(), parry.dirz(), true };
	Event ev{ EventType::EV_ACTION, ac };
	Framework::Get().eventQueue.push(ev);

	return true;
}
