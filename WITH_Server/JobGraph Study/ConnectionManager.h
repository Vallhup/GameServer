#pragma once

#include "Connection.h"
#include "SendBuffer.h"
#include "types.h"

class ConnectionManager {
public:
	void Add(Connection& conn);
	void Remove(Connection& conn);

	Connection* GetConnection(uint32 id);

	void Send(uint32 id, SendBuffer* data);
	void Broadcast(SendBuffer* data, uint32 expected);

private:
	std::mutex _mtx;
	std::unordered_map<uint32, std::shared_ptr<Connection>> _connections;
};