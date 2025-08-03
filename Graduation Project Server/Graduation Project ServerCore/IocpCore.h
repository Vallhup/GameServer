#pragma once

class IocpObject {
public:
	virtual ~IocpObject() = default;

public:
	virtual HANDLE GetHandle() const = 0;
	virtual void Dispatch(class ExpOver* expOver, int numOfBytes = 0) = 0;
};

class IocpCore {
	static constexpr ULONG maxEntry{ 32 };

public:
	IocpCore();
	~IocpCore();

	IocpCore(const IocpCore&) = delete;
	IocpCore& operator=(const IocpCore&) = delete;

	IocpCore(IocpCore&&) = delete;
	IocpCore& operator=(IocpCore&&) = delete;

public:
	bool Register(const std::shared_ptr<IocpObject>& iocpObject);
	bool Dispatch(unsigned int timeOutMs = INFINITE);

public:
	HANDLE GetHandle() const { return _iocpHandle; }

private:
	HANDLE _iocpHandle;
};

