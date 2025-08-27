#pragma once

#include "Protocols/Protocol.pb.h"

class IInputable;

class InputSystem {
public:
	using KeyState = std::bitset<Protocol::Input::MAX - 1>;

public:
	void Register(int id, IInputable* i);
	void Deregister(int id);

	void HandleInput(int id, const Protocol::CS_INPUT_PACKET& packet);

private:
	std::unordered_map<int, IInputable*> _inputables;
};

