#include "pch.h"
#include "NetworkManager.h"

void NetworkManager::Initialize(uint16 threadCnt, std::string_view ip, uint16 port, 
	IConnectionListener& listener)
{
	if (!_service)
	{
		_service = std::make_unique<ClientService>(threadCnt, ip, port, listener);
		_service->Start();
	}
}

void NetworkManager::Release()
{
	if (_service)
	{
		_service->Stop();
		_service.reset();
		_service = nullptr;
	}
}

bool NetworkManager::SendTimeSyncPacket(uint32_t probeSeq, uint32_t serverSendTimeMs)
{
	Protocol::CS_TIME_SYNC_PACKET timeSync;
	timeSync.set_probeseq(probeSeq);
	timeSync.set_echoedserversendtimems(serverSendTimeMs);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_TIME_SYNC_PACKET>(
		PacketType::CS_TIME_SYNC, timeSync);

	return TrySendInternal(data);
}

bool NetworkManager::SendLoginPacket(const std::string& id, const std::string& pw)
{
	Protocol::CS_LOGIN_PACKET login;
	login.set_loginid(id);
	login.set_password(pw);
	login.set_clientversion("1");

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_LOGIN_PACKET>(
		PacketType::CS_LOGIN, login);

	return TrySendInternal(data);
}

bool NetworkManager::SendCharacterSelectPacket(CharacterId id)
{
	Protocol::CS_CHARACTER_SELECT_PACKET characterSelect;
	characterSelect.set_characterid(static_cast<uint32_t>(id));

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_CHARACTER_SELECT_PACKET>(
		PacketType::CS_CHARACTER_SELECT, characterSelect);

	return TrySendInternal(data);
}

bool NetworkManager::SendMovePacket(int inputX, int inputZ, float yaw, bool isRun)
{
	Protocol::CS_MOVE_PACKET move;
	move.set_inputx(inputX);
	move.set_inputz(inputZ);
	move.set_yaw(yaw);
	move.set_isrun(isRun);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_MOVE_PACKET>(
		PacketType::CS_MOVE, move);
	
	return TrySendInternal(data);
}

bool NetworkManager::SendAttackPacket(float dirX, float dirZ, uint32_t animId, float normTime, uint32_t instanceId, Protocol::AttackInputType type)
{
	Protocol::CS_ATTACK_PACKET attack;
	attack.set_dirx(dirX);
	attack.set_dirz(dirZ);
	attack.set_clientanimid(animId);
	attack.set_clientnormalizedtime(normTime);
	attack.set_clientabilityinstanceid(instanceId);
	attack.set_attackinputtype(type);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_ATTACK_PACKET>(
		PacketType::CS_ATTACK, attack);

	return TrySendInternal(data);
}

bool NetworkManager::SendDodgePacket(float dirX, float dirZ)
{
	Protocol::CS_DODGE_PACKET dodge;
	dodge.set_dirx(dirX);
	dodge.set_dirz(dirZ);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_DODGE_PACKET>(
		PacketType::CS_DODGE, dodge);
	
	return TrySendInternal(data);
}

bool NetworkManager::SendGuardPacket(bool pressed)
{
	Protocol::CS_GUARD_PACKET guard;
	guard.set_input(pressed);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_GUARD_PACKET>(
		PacketType::CS_GUARD, guard);
	
	return TrySendInternal(data);
}

bool NetworkManager::SendParryPacket(float dirX, float dirZ)
{
	Protocol::CS_PARRY_PACKET parry;
	parry.set_dirx(dirX);
	parry.set_dirz(dirZ);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_PARRY_PACKET>(
		PacketType::CS_PARRY, parry);
	
	return TrySendInternal(data);
}

bool NetworkManager::SendUseItemPacket(float dirX, float dirZ)
{
	Protocol::CS_USE_ITEM_PACKET useItem;
	useItem.set_dirx(dirX);
	useItem.set_dirz(dirZ);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_USE_ITEM_PACKET>(
		PacketType::CS_USE_ITEM, useItem);

	return TrySendInternal(data);
}

bool NetworkManager::SendWorldTransitionRequestPacket(uint32_t requestId)
{
	Protocol::CS_WORLD_TRANSITION_REQUEST_PACKET transitionRequest;
	transitionRequest.set_requestid(requestId);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_WORLD_TRANSITION_REQUEST_PACKET>(
		PacketType::CS_WORLD_TRANSITION_REQUEST, transitionRequest);

	return TrySendInternal(data);
}

bool NetworkManager::SendWorldTransitionReadyPacket(uint64_t transferId)
{
	Protocol::CS_WORLD_TRANSITION_READY_PACKET transitionReady;
	transitionReady.set_transferid(transferId);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_WORLD_TRANSITION_READY_PACKET>(
		PacketType::CS_WORLD_TRANSITION_READY, transitionReady);
	
	return TrySendInternal(data);
}

bool NetworkManager::SendPartyUiOpenedPacket()
{
	Protocol::CS_PARTY_UI_OPENED_PACKET uiOpen;
	uiOpen.set_clientrequestid(_nextPartyRequestId++);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_PARTY_UI_OPENED_PACKET>(
		PacketType::CS_PARTY_UI_OPENED, uiOpen);

	return TrySendInternal(data);
}

bool NetworkManager::SendPartyUiClosedPacket()
{
	Protocol::CS_PARTY_UI_CLOSED_PACKET uiClose;
	uiClose.set_clientrequestid(_nextPartyRequestId++);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_PARTY_UI_CLOSED_PACKET>(
		PacketType::CS_PARTY_UI_CLOSED, uiClose);

	return TrySendInternal(data);
}

bool NetworkManager::SendPartyListRefreshPacket()
{
	Protocol::CS_PARTY_LIST_REFRESH_PACKET refresh;
	refresh.set_clientrequestid(_nextPartyRequestId++);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_PARTY_LIST_REFRESH_PACKET>(
		PacketType::CS_PARTY_LIST_REFRESH, refresh);

	return TrySendInternal(data);
}

bool NetworkManager::SendPartyCreatePacket()
{
	Protocol::CS_PARTY_CREATE_PACKET create;
	create.set_clientrequestid(_nextPartyRequestId++);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_PARTY_CREATE_PACKET>(
		PacketType::CS_PARTY_CREATE, create);

	return TrySendInternal(data);
}

bool NetworkManager::SendPartyJoinRequestPacket(uint64_t partyId)
{
	Protocol::CS_PARTY_JOIN_REQUEST_PACKET joinRequest;
	joinRequest.set_clientrequestid(_nextPartyRequestId++);
	joinRequest.set_partyid(partyId);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_PARTY_JOIN_REQUEST_PACKET>(
		PacketType::CS_PARTY_JOIN_REQUEST, joinRequest);

	return TrySendInternal(data);
}

bool NetworkManager::SendPartyJoinAcceptPacket(uint64_t joinRequestId)
{
	Protocol::CS_PARTY_JOIN_ACCEPT_PACKET joinAccept;
	joinAccept.set_clientrequestid(_nextPartyRequestId++);
	joinAccept.set_joinrequestid(joinRequestId);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_PARTY_JOIN_ACCEPT_PACKET>(
		PacketType::CS_PARTY_JOIN_ACCEPT, joinAccept);

	return TrySendInternal(data);
}

bool NetworkManager::SendPartyJoinRejectPacket(uint64_t joinRequestId)
{
	Protocol::CS_PARTY_JOIN_REJECT_PACKET joinReject;
	joinReject.set_clientrequestid(_nextPartyRequestId++);
	joinReject.set_joinrequestid(joinRequestId);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_PARTY_JOIN_REJECT_PACKET>(
		PacketType::CS_PARTY_JOIN_REJECT, joinReject);

	return TrySendInternal(data);
}

bool NetworkManager::SendRespawnRequestPacket()
{
	Protocol::CS_RESPAWN_REQUEST_PACKET respawn;
	respawn.set_clientrequestid(_nextRespawnRequestId++);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_RESPAWN_REQUEST_PACKET>(
		PacketType::CS_RESPAWN_REQUEST, respawn);

	return TrySendInternal(data);
}

bool NetworkManager::SendStatUiOpenedPacket()
{
	Protocol::CS_STAT_UI_OPENED_PACKET packet;
	packet.set_clientrequestid(_nextTitleRequestId++);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_STAT_UI_OPENED_PACKET>(
			PacketType::CS_STAT_UI_OPENED, packet);

	return TrySendInternal(data);
}

bool NetworkManager::SendTitleEquipRequestPacket(uint32_t titleId)
{
	Protocol::CS_TITLE_EQUIP_REQUEST_PACKET packet;
	packet.set_clientrequestid(_nextTitleRequestId++);
	packet.set_titleid(titleId);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_TITLE_EQUIP_REQUEST_PACKET>(
			PacketType::CS_TITLE_EQUIP_REQUEST, packet);

	return TrySendInternal(data);
}

bool NetworkManager::SendFinalClearChoiceSubmit(uint64_t voteId, bool choosePvp)
{
	Protocol::CS_FINAL_CLEAR_CHOICE_SUBMIT_PACKET packet;
	packet.set_voteid(voteId);
	packet.set_choosepvp(choosePvp);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_FINAL_CLEAR_CHOICE_SUBMIT_PACKET>(
			PacketType::CS_FINAL_CLEAR_CHOICE_SUBMIT, packet);

	return TrySendInternal(data);
}

bool NetworkManager::TrySendInternal(SendBuffer* sendBuffer)
{
	if (_service && sendBuffer)
	{
		_service->Send(sendBuffer);
		return true;
	}

	return false;
}
