#pragma once

class ServerConnectionListener;

class SessionSendBufferManager {
public:
	explicit SessionSendBufferManager(ServerConnectionListener& listener)
		: _listener(listener) {}

	void Enqueue(uint32_t connId, const void* data, uint32_t len);
	void FlushOne(uint32_t connId);
	void FlushAll();
	void ReleaseSession(uint32_t connId);

private:
	ServerConnectionListener& _listener;
	std::vector<SendBuffer*> _buffers;
};