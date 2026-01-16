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

	MoveEvent mv{ id, move.inputx(), move.inputz(), move.yaw() };
	Event ev{ EventType::EV_MOVE, mv };
	Framework::Get().eventQueue.push(ev);

	return true;
}

bool NetworkHandler::HandleAttack(uint32 id, const PacketHeader& header, const BYTE* data)
{
	Protocol::CS_ATTACK_PACKET attack;
	if (not PacketFactory::Deserialize(header, data, &attack))
		return false;

	ActionEvent ac{ id, ActionRequestType::Attack, attack.dirx(), attack.dirz() };
	Event ev{ EventType::EV_ACTION, ac };
	Framework::Get().eventQueue.push(ev);

	return true;
}

bool NetworkHandler::HandleDodge(uint32 id, const PacketHeader& header, const BYTE* data)
{
	Protocol::CS_DODGE_PACKET dodge;
	if (not PacketFactory::Deserialize(header, data, &dodge))
		return false;

	ActionEvent ac{ id, ActionRequestType::Dodge, dodge.dirx(), dodge.dirz() };
	Event ev{ EventType::EV_ACTION, ac };
	Framework::Get().eventQueue.push(ev);

	return true;
}
