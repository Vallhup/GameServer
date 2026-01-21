#pragma once

#include "SendBuffer.h"

class IConnectionListener;

class Connection 
	: public std::enable_shared_from_this<Connection> {
	static constexpr size_t MAX_BUFFERS{ 32 };
	static constexpr size_t MAX_BYTES{ 64 * 1024 };
	static constexpr uint32 DEFAULT_MAX_PACKET_SIZE{ 1024 * 1024 };

public:
	explicit Connection(tcp::socket s, IConnectionListener& listener
		, uint32 maxPacketSize = DEFAULT_MAX_PACKET_SIZE);
	virtual ~Connection() = default;

	void Start();
	void Stop();

	void Send(SendBuffer* data);

	uint32 GetId() const { return _id; }
	void SetId(uint32 id) { _id = id; }

private:
	void DoRecv();
	void DoSend();

	void OnRecv(std::error_code ec, size_t bytes);
	void OnSend(std::error_code ec, size_t batchCount);

	void ProcessPacket();
	void Close();

	uint32 _id;

	bool _isClosed;
	tcp::socket _socket;
	asio::strand<asio::any_io_executor> _strand;

	const uint32 _maxPacketSize;
	IConnectionListener& _listener;

	std::array<BYTE, 4096> _recvTemp;
	std::vector<BYTE> _recvAccum;

	bool _isSending;
	std::deque<SendBuffer*> _sendQueue;
	std::vector<asio::const_buffer> _gatherBufs;
};

