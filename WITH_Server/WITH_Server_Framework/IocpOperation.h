#pragma once

struct SendBuffer;

class IocpAcceptor;
class IocpConnection;

struct IocpOperation
{
	OVERLAPPED ov{ 0, 0, 0, 0, nullptr };
	void (*dispatch)(IocpOperation* self, DWORD bytes, bool success) { nullptr };
};

struct RecvOp : IocpOperation
{
	WSABUF			wsaBufs[2]{};
	IocpConnection* owner{ nullptr };

	void Init(IocpConnection* conn) noexcept;
};

struct SendOp : IocpOperation
{
	std::vector<WSABUF>			wsaBufs;
	std::vector<SendBuffer*> ownedBuffers;
	IocpConnection*				owner{ nullptr };

	void Init(IocpConnection* conn) noexcept;
	void Reset() noexcept;
};

struct AcceptOp : IocpOperation
{
	// (sizeof(SOCKADDR_IN6) + 16) * 2 = 88 - IPv6 호환
	char			addrBuf[88]{};
	SOCKET			socket{ INVALID_SOCKET };
	IocpAcceptor*	owner{ nullptr };

	void Init(IocpAcceptor* acceptor, SOCKET preparedSocket) noexcept;
};


template<typename Handler>
concept IocpHandlerConcept = std::invocable<Handler, DWORD, bool>;

template<IocpHandlerConcept Handler>
struct IocpOperationImpl : IocpOperation
{
	Handler handler;

	explicit IocpOperationImpl(Handler h) : handler(std::move(h))
	{
		dispatch =
			[](IocpOperation* base, DWORD bytes, bool success)
			{
				auto* self = static_cast<IocpOperationImpl<Handler>*>(base);
				self->handler(bytes, success);
				delete self;
			};
	}
};

template<IocpHandlerConcept Handler>
[[nodiscard]] IocpOperation* MakeIocpOperation(Handler&& h)
{
	return new IocpOperationImpl<std::decay_t<Handler>>(std::forward<Handler>(h));
}