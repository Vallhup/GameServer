#pragma once

#include "ExpOver.h"
#include "IocpCore.h"
#include "GameObject.h"

enum State : char {
	ST_ALLOC,
	ST_INGAME,
	ST_FREE
};

constexpr int VIEW_RANGE = 7;

class RecvOver;
class SendOver;
class Service;

class Session : public IocpObject
{
	friend class Listener;
	friend class IocpCore;
	friend class Service;

public:
	Session()
	{
		_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, NULL, WSA_FLAG_OVERLAPPED);
	}

	virtual ~Session();

public:
	void Send(const std::vector<char>& data);
	virtual bool ProcessPacket(const std::vector<char>& packet) abstract;

public:
	// Getter
	SOCKET GetSocket() const { return _socket; }

	// Setter
	void SetService(std::shared_ptr<Service> service) { _service = service; }

public:
	void doRecv();
	void doSend();

	void RecvCallback(DWORD numBytes);
	void SendCallback();

protected:
	void Close();

private:
	// Interface ±¸Çö
	virtual HANDLE GetHandle() override;
	virtual void Dispatch(ExpOver* expOver, int numOfBytes = 0) override;

protected:
	std::atomic<State> _state{ ST_ALLOC };
	std::weak_ptr<Service> _service;
	SOCKET	_socket;

protected:
	std::atomic<int> _pendingIoCount{ 0 };
	std::atomic<bool> _shouldRelease{ false };

protected:
	AtomicQueue<std::vector<char>> _sendQueue;
	std::atomic<bool> _isSending{ false };

protected:
	RecvOver	_recvOver;
	SendOver	_sendOver;
};

class GameSession : 
	public Session, 
	public GameObject,
	public std::enable_shared_from_this<GameSession> {

	friend class Service;

public:
	GameSession();
	virtual ~GameSession() override;

public:
	virtual bool ProcessPacket(const std::vector<char>& packet) override;
	virtual bool IsVisible() const override { return _state == ST_INGAME; }

public:
	bool HandleLogin(const std::vector<char>& packet, std::shared_ptr<Service> service);
	bool HandleMove(const std::vector<char>& packet, std::shared_ptr<Service> service);
	bool HandleChat(const std::vector<char>& packet, std::shared_ptr<Service> service);

protected:
	std::unordered_set<int>		_viewList;
	mutable std::shared_mutex	_viewLock;
};