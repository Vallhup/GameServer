#pragma once

class Service;

class ChatManager
{
public:
	ChatManager(Service& service) : _service(service) {}

	void HandleMessage(short senderId, const char* msg);

private:
	Service& _service;
};

