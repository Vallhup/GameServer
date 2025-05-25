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
class Party;
class NPC;
class Inventory;

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
	virtual void Dispatch(ExpOver* expOver, int numOfBytes = 0) abstract;

protected:
	std::atomic<State> _state{ ST_ALLOC };
	std::weak_ptr<Service> _service;
	SOCKET	_socket;

protected:
	std::atomic<int> _pendingIoCount{ 0 };
	std::atomic<bool> _shouldRelease{ false };

protected:
	concurrency::concurrent_queue<std::shared_ptr<std::vector<char>>> _sendQueue;
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
	friend class NPC;

public:
	GameSession();
	virtual ~GameSession() override;

public:
	virtual void Dispatch(ExpOver* expOver, int numOfBytes = 0) override;

public:
	virtual bool ProcessPacket(const std::vector<char>& packet) override;
	virtual bool IsVisible() const override { return _state == ST_INGAME; }

private:
	void OnHeal();

public:
	std::shared_ptr<GameSession> ConsumePendingPartyRequester();

public:
	std::shared_ptr<Inventory>& GetInventory() { return _inventory; }

	void SetParty(std::shared_ptr<Party> party) { _party = party; }
	void SetInventory(std::shared_ptr<Inventory> inventory) { _inventory = inventory; }

private:
	std::atomic<std::shared_ptr<std::unordered_set<int>>> _viewList;

private:
	std::weak_ptr<Party> _party;
	std::atomic<std::weak_ptr<GameSession>> _pendingPartyRequester;

private:
	std::shared_ptr<Inventory> _inventory;
};