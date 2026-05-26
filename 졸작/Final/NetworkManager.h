#pragma once

#include "EntityId.h"
#include "ClientService.h"
#include "../../Network_Library/Include/SendBuffer.h"
#include <string>

class NetworkManager {
public:
	NetworkManager() : _service(nullptr), _nextPartyRequestId(1) {}

	void Initialize(
		uint16 threadCnt, 
		std::string_view ip, 
		uint16 port, 
		IConnectionListener& listener);

	void Release();

	bool SendTimeSyncPacket(uint32_t probeSeq, uint32_t serverSendTimeMs);

	bool SendLoginPacket(const std::string& id, const std::string& pw);
	bool SendCharacterSelectPacket(CharacterId id);
	bool SendMovePacket(int inputX, int inputZ, float yaw, bool isRun);
	bool SendAttackPacket(float dirX, float dirZ, uint32_t animId, float normTime, uint32_t instanceId, Protocol::AttackInputType type);
	bool SendDodgePacket(float dirX, float dirZ);
	bool SendGuardPacket(bool pressed);
	bool SendParryPacket(float dirX, float dirZ);
	bool SendUseItemPacket(float dirX, float dirZ);

	bool SendWorldTransitionRequestPacket(uint32_t requestId);
	bool SendWorldTransitionReadyPacket(uint64_t transferId);

	bool SendPartyUiOpenedPacket();
	bool SendPartyUiClosedPacket();
	bool SendPartyListRefreshPacket();
	bool SendPartyCreatePacket();
	bool SendPartyJoinRequestPacket(uint64_t partyId);
	bool SendPartyJoinAcceptPacket(uint64_t joinRequestId);
	bool SendPartyJoinRejectPacket(uint64_t joinRequestId);

private:
	bool TrySendInternal(SendBuffer* sendBuffer);

	std::unique_ptr<ClientService> _service;
	uint32_t _nextPartyRequestId;
};