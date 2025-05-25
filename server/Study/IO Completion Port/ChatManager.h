#pragma once

class Service;

class ChatManager
{
public:
	ChatManager() = default;

	void SetService(std::shared_ptr<Service> service) { _service = service; }

	void HandleMessage(short senderId, const char* msg);

private:
	std::weak_ptr<Service> _service;
};

