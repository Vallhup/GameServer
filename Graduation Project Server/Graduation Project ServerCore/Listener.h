#pragma once

class IGameContext;

class Listener : public IocpObject {
	static constexpr u_short PORT_NUM{ 7000 };
public: 
	Listener() = delete;
	Listener(IGameContext& gameCtx);
	virtual ~Listener();
	
public:
	virtual HANDLE GetHandle() const override;
	virtual void Dispatch(class ExpOver* expOver, int numOfBytes = 0) override;

public:
	bool Init(u_short portNum = PORT_NUM);
	bool Start();
	void Stop();

private:
	void RegisterAccept(AcceptOver* acceptOver);
	void ProcessAccept(AcceptOver* acceptOver);

private:
	IGameContext& _gameCtx;

	SOCKET _socket;
	std::vector<AcceptOver*> _acceptOvers;

	std::atomic<bool> _accepting;
};

