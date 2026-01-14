#pragma once

class IAcceptListener {
public:
	virtual ~IAcceptListener() = default;

	virtual void OnAccept(tcp::socket s) = 0;
};

