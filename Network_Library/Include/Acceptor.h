#pragma once

class IAcceptListener;

class Acceptor {
public:
	Acceptor() = delete;
	Acceptor(asio::io_context& io, uint16 port, IAcceptListener& listener);

	void Start();
	void Stop();

private:
	void DoAccept();
	void Close();

	bool _isRunning;
	tcp::acceptor _acceptor;
	asio::strand<asio::any_io_executor> _strand;

	IAcceptListener& _listener;
};

