#pragma once

#include "EntityId.h"
#include "ClientService.h"

class NetworkManager {
public:
	NetworkManager() : _service(nullptr) {}

	void Initialize(
		uint16 threadCnt, 
		std::string_view ip, 
		uint16 port, 
		IConnectionListener& listener);

	void Release();

	bool SendLoginPacket();
	bool SendCharacterSelectPacket(CharacterId id);
	bool SendMovePacket(int inputX, int inputZ, float yaw, bool isRun);
	bool SendAttackPacket(float dirX, float dirZ);
	bool SendDodgePacket(float dirX, float dirZ);
	bool SendGuardPacket(bool pressed);
	bool SendParryPacket(float dirX, float dirZ);

	bool SendWorldTransitionRequestPacket(uint32_t requestId);
	bool SendWorldTransitionReadyPacket(uint64_t transferId);

private:
	std::unique_ptr<ClientService> _service;
};