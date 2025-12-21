#pragma once

#define ASIO_STANDALONE
#include <asio.hpp>
#include <unordered_map>
#include <memory>

#include "Listener.h"
#include "Session.h"

class Network {
	friend class Listener;

public:
	Network(short port, int size);
	~Network();

	void Start();
	void Stop();

	void Send(int id, const void* data);
	void Broadcast(const void* data, int exepted = -1);

private:
	void AddSession(int id, const std::shared_ptr<Session>& session);
	void RemoveSession(int id);

	std::atomic<int> _nextId;
	asio::io_context _ioCtx;
	asio::executor_work_guard<asio::io_context::executor_type> _workGuard;
	Listener _listener;

	std::mutex _mtx;
	std::unordered_map<int, std::shared_ptr<Session>> _sessions;
	// TEMP : TBB - concurrent_unordered_map 으로 변경

	std::vector<std::thread> _workers;
};

// 개선점
//
// 1. boost::asio를 사용하면 shared_ptr의 사용이 강제된다
//  - custom atomic_shared_ptr의 구현
//  - 흠...