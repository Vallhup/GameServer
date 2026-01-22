#pragma once

struct SendBuffer;
class Connection;

class ConnectionRegistry {
public:
	void Add(const std::shared_ptr<Connection> c);
	void Remove(uint32 id);

	std::shared_ptr<Connection> GetConnection(uint32 id);
	
	void Send(uint32 id, SendBuffer* data);
	void Broadcast(SendBuffer* data, uint32 expected);
	
private:
	concurrency::concurrent_unordered_map<uint32,
		std::atomic<std::shared_ptr<Connection>>> _connections;
};