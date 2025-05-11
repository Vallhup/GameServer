#pragma once

class IocpObject : public std::enable_shared_from_this<IocpObject>
{
public:
	virtual HANDLE GetHandle() abstract;
	virtual void Dispatch(class ExpOver* expOver, int nuOfBytes = 0) abstract;
	
	void SetId(int id) { _id = id; };

protected:
	int _id;
};

class IocpCore
{
public:
	IocpCore();
	~IocpCore();

public:
	HANDLE GetHandle() { return _iocpHandle; };

public:
	bool Register(std::shared_ptr<IocpObject> iocpObject);
	bool Dispatch(unsigned int timeoutMs = INFINITE);

private:
	HANDLE _iocpHandle;

	static int clientId;
};

