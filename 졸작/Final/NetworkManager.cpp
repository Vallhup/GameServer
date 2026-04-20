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

bool NetworkManager::SendLoginPacket()
{
	if (_service == nullptr)
	{
		return false;
	}

	Protocol::CS_LOGIN_PACKET login;

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_LOGIN_PACKET>(
		PacketType::CS_LOGIN, login);

	if (data != nullptr)
	{
		_service->Send(data);
		return true;
	}
	else
	{
		return false;
	}
}

bool NetworkManager::SendMovePacket(int inputX, int inputZ, float yaw, bool isRun)
{
	if (_service == nullptr)
	{
		return false;
	}

	Protocol::CS_MOVE_PACKET move;
	move.set_inputx(inputX);
	move.set_inputz(inputZ);
	move.set_yaw(yaw);
	move.set_isrun(isRun);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_MOVE_PACKET>(
		PacketType::CS_MOVE, move);
	
	if (data != nullptr)
	{
		_service->Send(data);
		return true;
	}
	else
	{
		return false;
	}
}

bool NetworkManager::SendAttackPacket(float dirX, float dirZ)
{
	if (_service == nullptr)
	{
		return false;
	}

	Protocol::CS_ATTACK_PACKET attack;
	attack.set_dirx(dirX);
	attack.set_dirz(dirZ);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_ATTACK_PACKET>(
		PacketType::CS_ATTACK, attack);
	
	if (data != nullptr)
	{
		_service->Send(data);
		return true;
	}
	else
	{
		return false;
	}
}

bool NetworkManager::SendDodgePacket(float dirX, float dirZ)
{
	if (_service == nullptr)
	{
		return false;
	}

	Protocol::CS_DODGE_PACKET dodge;
	dodge.set_dirx(dirX);
	dodge.set_dirz(dirZ);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_DODGE_PACKET>(
		PacketType::CS_DODGE, dodge);
	
	if (data != nullptr)
	{
		_service->Send(data);
		return true;
	}
	else
	{
		return false;
	}
}

bool NetworkManager::SendGuardPacket(bool pressed)
{
	if (_service == nullptr)
	{
		return false;
	}

	Protocol::CS_GUARD_PACKET guard;
	guard.set_input(pressed);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_GUARD_PACKET>(
		PacketType::CS_GUARD, guard);
	
	if (data != nullptr)
	{
		_service->Send(data);
		return true;
	}
	else
	{
		return false;
	}
}

bool NetworkManager::SendParryPacket(float dirX, float dirZ)
{
	if (_service == nullptr)
	{
		return false;
	}

	Protocol::CS_PARRY_PACKET parry;
	parry.set_dirx(dirX);
	parry.set_dirz(dirZ);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_PARRY_PACKET>(
		PacketType::CS_PARRY, parry);
	
	if (data != nullptr)
	{
		_service->Send(data);
		return true;
	}
	else
	{
		return false;
	}
}

bool NetworkManager::SendWorldTransitionRequestPacket(uint32_t requestId)
{
	if (_service == nullptr)
	{
		return false;
	}

	Protocol::CS_WORLD_TRANSITION_REQUEST_PACKET transitionRequest;
	transitionRequest.set_requestid(requestId);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_WORLD_TRANSITION_REQUEST_PACKET>(
		PacketType::CS_WORLD_TRANSITION_REQUEST, transitionRequest);
	
	if (data != nullptr)
	{
		_service->Send(data);
		return true;
	}
	else
	{
		return false;
	}
}

bool NetworkManager::SendWorldTransitionReadyPacket(uint64_t transferId)
{
	if (_service == nullptr)
	{
		return false;
	}

	Protocol::CS_WORLD_TRANSITION_READY_PACKET transitionReady;
	transitionReady.set_transferid(transferId);

	SendBuffer* data = PacketFactory::Serialize<Protocol::CS_WORLD_TRANSITION_READY_PACKET>(
		PacketType::CS_WORLD_TRANSITION_READY, transitionReady);
	
	if (data != nullptr)
	{
		_service->Send(data);
		return true;
	}
	else
	{
		return false;
	}
}
