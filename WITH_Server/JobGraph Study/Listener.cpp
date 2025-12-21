#include "Listener.h"
#include "Network.h"

#include <iostream>

Listener::Listener(asio::io_context& io, short port)
	: _acceptor(io, tcp::endpoint(tcp::v4(), port))
{
	_acceptor.set_option(asio::socket_base::reuse_address(true));

#ifdef _DEBUG
	std::cout << "Listening on port " << port << "..." << std::endl;
#endif
}

void Listener::Start(Network* net)
{
	Accept(net);

#ifdef _DEBUG
	std::cout << "Listener Start" << std::endl;
#endif
}

void Listener::Stop()
{
	std::error_code ec;
	_acceptor.close(ec);

#ifdef _DEBUG
	if (ec)
		std::cerr << "Listener Stop error: " << ec.message() << std::endl;
#endif
}

void Listener::Accept(Network* net)
{
	_acceptor.async_accept(
		[this, net](std::error_code ec, tcp::socket socket)
		{
			if (not ec)
			{
				int id = net->_nextId.fetch_add(1);

				socket.set_option(tcp::no_delay(true));
				auto session = std::make_shared<Session>(std::move(socket), id);
				net->AddSession(id, session);
				session->Start();
			}
#ifdef _DEBUG
			else
				std::cerr << "Accept error: " << ec.message() << std::endl;
#endif
			Accept(net);
		});
}
