#pragma once

class IocpObject : public std::enable_shared_from_this<IocpObject>
{
public:
	virtual HANDLE GetHandle() abstract;
	virtual void Dispatch(class ExpOver* expOver, int nuOfBytes = 0) abstract;
};

class IocpCore
{
};

