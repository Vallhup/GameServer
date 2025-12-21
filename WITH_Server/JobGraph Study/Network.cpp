#include "Network.h"
#include "Protocol.hpp"
#include <iostream>

Network::Network(short port, int size)
	: _ioCtx(4), _workGuard(asio::make_work_guard(_ioCtx)),
	_listener(_ioCtx, port), _nextId(0)
{
}

Network::~Network()
{
}

void Network::Start()
{
	_listener.Start(this);

	for (int i = 0; i < 4; ++i)
	{
		_workers.emplace_back(
			[this]()
			{
				_ioCtx.run();
			});
	}
}

void Network::Stop()
{
	_workGuard.reset();
	_listener.Stop();
	_ioCtx.stop();

	for (auto& worker : _workers)
	{
		if (worker.joinable())
			worker.join();
	}
}

void Network::Send(int id, const void* data)
{
	std::lock_guard lock{ _mtx };
	auto it = _sessions.find(id);
	if (it != _sessions.end())
	{
		const PacketHeader* header = reinterpret_cast<const PacketHeader*>(data);
		it->second->Send(data, header->size);
	}
}

void Network::Broadcast(const void* data, int exepted)
{
	std::lock_guard lock{ _mtx };
	for (auto& [id, session] : _sessions)
	{
		if (exepted == id) continue;
		const PacketHeader* header = reinterpret_cast<const PacketHeader*>(data);
		session->Send(data, header->size);
	}
}

void Network::AddSession(int id, const std::shared_ptr<Session>& session)
{
	_sessions[id] = session;
}

void Network::RemoveSession(int id)
{
	_sessions.erase(id);
}
