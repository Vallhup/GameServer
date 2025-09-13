#pragma once

class GameObject;

enum class SessionState : char {
	ST_ALLOC,
	ST_INGAME,
	ST_FREE
};

class Session : public IocpObject {
	static constexpr int MAX_PACKET{ 32 };

	using PacketHandler = std::function<void(int, const std::vector<char>&)>;

public:
	Session() = delete;
	Session(int id, SOCKET socket, ISessionManager* owner);
	virtual ~Session();

public:
	virtual HANDLE GetHandle() const override;
	virtual void Dispatch(class ExpOver* expOver, int numOfBytes = 0) override;

public:
	void RegisterRecv();
	void RegisterSend(const std::vector<char>& data);

	void ProcessRecv(DWORD numBytes);
	void ProcessSend();

	void DisConnect();

public:
	int GetId() const { return _id; }
	SessionState GetState() const { return _state.load(); }
	GameObject* GetCharacter() const { return _character; }

	void SetPacketHandler(PacketHandler handler) { _packetHandler = handler; }
	void SetCharacter(GameObject* character) { _character = character; }
	void SetState(SessionState state) { _state = state; }

private:
	void InternalSend();

private:
	int _id;

	SOCKET _socket;
	RecvOver _recvOver;

	concurrency::concurrent_queue<std::vector<char>> _sendQueue;
	std::atomic<bool> _isSending;

	PacketHandler _packetHandler;

	std::atomic<bool> _connected;
	std::atomic<SessionState> _state;

	GameObject* _character;
	ISessionManager* _owner;
};

