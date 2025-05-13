#pragma once

#include "ExpOver.h"
#include "IocpCore.h"

struct Pos
{
	short _xPos;
	short _yPos;
};

enum State : char {
	ST_ALLOC,
	ST_INGAME,
	ST_FREE
};

constexpr int VIEW_RANGE = 7;

// Client의 정보 (고유 id, 연결 socket 등)
// 컨텐츠와 관련된 작업들
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
	int    GetSessionId() const { return _id; }

	// Setter
	void SetService(std::shared_ptr<Service> service) { _service = service; }
	void SetId(int id) { _id = id; };

public:
	void doRecv();
	void doSend();

	void RecvCallback(DWORD numBytes);
	void SendCallback();

private:
	void Close();

private:
	// Interface 구현
	virtual HANDLE GetHandle() override;
	virtual void Dispatch(ExpOver* expOver, int numOfBytes = 0) override;

protected:
	std::atomic<State> _state{ ST_ALLOC };

protected:
	std::weak_ptr<Service> _service;
	SOCKET	_socket;

protected:
	AtomicQueue<std::vector<char>> _sendQueue;
	std::atomic<bool> _isSending{ false };

protected:
	RecvOver	_recvOver;
	SendOver	_sendOver;
};

class GameSession : public Session {
	friend class Service;

public:
	virtual ~GameSession() override;

public:
	virtual bool ProcessPacket(const std::vector<char>& packet) override;

public:
	// Send 관련
	// target의 Add, Move, Remove, Login을 자신의 client에게 Send하는 함수
	void sendAddPlayerPacket(const std::shared_ptr<GameSession>& target);
	void sendMovePacket(const std::shared_ptr<GameSession>& target);
	void sendRemovePacket(const std::shared_ptr<GameSession>& target);
	void sendLoginPacket(const std::shared_ptr<GameSession>& target);

public:
	// view 관련
	// 나에게 target이 보이는지 여부를 return하는 함수
	bool can_see(const std::shared_ptr<GameSession>& target);

	std::unordered_set<int> collectViewList();
	std::unordered_set<int> updateViewList(const std::unordered_set<int>& newList);
	void syncViewList(const std::unordered_set<int>& oldList, const std::unordered_set<int>& newList);

public:
	// Getter
	Pos	   GetPos() const { return _pos; }

private:
	Pos		_pos{ rand() % 400, rand() % 400 };
	std::string _name;

private:
	std::unordered_set<int> _viewList;
	std::mutex				_viewLock;
};