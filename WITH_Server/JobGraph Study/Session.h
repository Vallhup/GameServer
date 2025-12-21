#pragma once

#define ASIO_STANDALONE
#include <asio.hpp>
#include <memory>
#include <deque>
#include <concurrent_vector.h>

#include "RecvBuffer.h"

using asio::ip::tcp;

class SendBuffer;

class Session : public std::enable_shared_from_this<Session> {
	static constexpr size_t MAX_BUFFERS{ 32 };
	static constexpr size_t MAX_BYTES{ 64 * 1024 };

public:
	explicit Session(tcp::socket s, int id);

	void Start();
	void Close();

	void Send(const void* data, uint16_t size);

private:
	void Recv();
	void InternalSend();
	void ProcessPacket();

	int _id;
	tcp::socket _socket;
	asio::strand<asio::any_io_executor> _strand;

	RecvBuffer _recvBuffer;
	std::deque<SendBuffer*> _sendQueue;
	std::vector<asio::const_buffer> _gatherBufs;
};