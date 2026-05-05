#pragma once

#include "IocpOperation.h"
#include "ObjectPool.hpp"

class IocpAcceptor {
	static constexpr uint32_t kPrePostCount{ 16 };

public:
	using OnAcceptFn = std::function<void(SOCKET)>;

	IocpAcceptor() = delete;
	IocpAcceptor(HANDLE iocpHandle, uint16_t port, OnAcceptFn onAccept);

	IocpAcceptor(const IocpAcceptor&)				= delete;
	IocpAcceptor& operator=(const IocpAcceptor&)	= delete;

	bool Start() noexcept;
	void Stop() noexcept;

	void OnAcceptComplete(AcceptOp* op, DWORD bytes, bool success) noexcept;

private:
	void RegisterAccept(AcceptOp* op) noexcept;

	SOCKET		_listenSocket{ INVALID_SOCKET };
	HANDLE		_iocpHandle{ INVALID_HANDLE_VALUE };

	uint16_t	_port;
	OnAcceptFn	_onAccept;

	LPFN_ACCEPTEX	_acceptEx{ nullptr };

	ObjectPool<AcceptOp, 32> _pool;
	std::atomic<bool> _running{ false };
};
