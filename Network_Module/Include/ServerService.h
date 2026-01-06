#pragma once

#include "NetworkService.h"
#include "Acceptor.h"
#include "IAcceptListener.h"

class IConnectionListener;

class ServerService 
	: public NetworkService, 
	  public IAcceptListener {
public:
	explicit ServerService(uint16 threadCnt, uint16 port, 
		IConnectionListener& connListener);
	virtual ~ServerService() = default;

	virtual void OnAccept(tcp::socket s) override = 0;

protected:
	virtual void OnStart() override;
	virtual void OnStop() override;

	Acceptor _acceptor;
	IConnectionListener& _connListener;
};