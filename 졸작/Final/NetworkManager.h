#pragma once

class NetworkManager {
	static constexpr u_short PORT_NUM{ 7000 };
public:
	NetworkManager() = default;
	~NetworkManager();

public:
	void Initialize(const char* IP, u_short port = PORT_NUM);
	void Update();
	void Release();

	void Send(const std::vector<char>& packet);

public:
	bool IsConnected() const { return isConnected; }

private:
	void ProcessPacket(const std::vector<char>& packet);

private:
	SOCKET clientSocket;
	bool isConnected;

	std::vector<char> recvBuffer;
};

