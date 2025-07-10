#pragma once

class Service;

class Listener : public std::enable_shared_from_this<Listener>
{
public:
	Listener(const std::shared_ptr<Service>& service);
	~Listener();

public:
	bool Init();
	SOCKET Accept();

public:
	SOCKET GetSocket() const;

private:
	SOCKET _socket{ INVALID_SOCKET };
	std::weak_ptr<Service> _service;
};

