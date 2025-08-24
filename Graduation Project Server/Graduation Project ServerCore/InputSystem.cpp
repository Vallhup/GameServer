#include "pch.h"
#include "InputSystem.h"

void InputSystem::Register(int id, IInputable* i)
{
	_inputables.try_emplace(id, i);
}

void InputSystem::Deregister(int id)
{
	_inputables.erase(id);
}

void InputSystem::HandleInput(const Protocol::CS_INPUT_PACKET& packet)
{
	auto it = _inputables.find(packet.header().sessionid());
	if (it != _inputables.end()) {
		it->second->HandleInput(packet);
	}
}