#include "Server.hpp"

#include <../connection/Connection.hpp>
#include <iostream>

Server::Server(asio::io_context& io, unsigned short port)
    : m_io(io),
      m_acceptor(io,
        asio::ip::tcp::endpoint(
            asio::ip::tcp::v4(),
            port
        )
    )
{
}

void Server::start()
{
    accept();
}

void Server::accept()
{
    auto connection = std::make_shared<Connection>(m_io);

    m_acceptor.async_accept(
        connection->socket(),
        [this, connection](const asio::error_code& ec)
        {
            if (ec)
            {
                std::cerr << "Accept error: " << ec.message() << std::endl;
            }
            else
            {
                std::cout << "Client connected!" << std::endl;
                connection->start();
            }
            accept(); // not stack overflow - not recursive in the traditional sense
        }
    );
}
