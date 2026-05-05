#pragma once

class IocpHandle {
public:
	IocpHandle() noexcept;
	~IocpHandle() noexcept;

	IocpHandle(const IocpHandle&)				= delete;
	IocpHandle& operator=(const IocpHandle&)	= delete;

	bool Register(HANDLE handle, ULONG_PTR key) noexcept;
	void WakeWorker() noexcept;

	HANDLE GetHandle() const noexcept { return _handle; }

private:
	HANDLE _handle{ INVALID_HANDLE_VALUE };
};

