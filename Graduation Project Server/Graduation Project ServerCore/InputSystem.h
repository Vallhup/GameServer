#pragma once

#include "Protocols/Protocol.pb.h"

class IInputable;

class InputSystem {
public:
	using KeyState = std::bitset<256>;

public:
	void Register(int id, IInputable* i);
	void Deregister(int id);

	void HandleInput(const Protocol::CS_INPUT_PACKET& packet);

private:
	std::unordered_map<int, IInputable*> _inputables;
};

