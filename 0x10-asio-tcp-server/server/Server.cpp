#include "Server.hpp"

#include <../connection/Connection.hpp>
#include <iostream>

#include "../debug/debug.h"

Server::Server(asio::io_context& io, unsigned short port)
    : m_io(io),
      m_acceptor(io,
        asio::ip::tcp::endpoint(
            asio::ip::tcp::v4(),
            port
        )
    )
{
    DEBUG_PRINT("Server created, this= " << this);
}

void Server::start()
{
    DEBUG_PRINT("Server starting, this= " << this);
    accept();
}

void Server::accept()
{
    auto connection = std::make_shared<Connection>(m_io);
    DEBUG_PRINT("Server->accept method started, this= " << connection.get() << " use_count= " << connection.use_count());

    m_acceptor.async_accept(
        connection->socket(),
        [this, connection](const asio::error_code& ec)
        {
            DEBUG_PRINT("Server->accept callback started, this= " << connection.get() << " use_count= " << connection.use_count());
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
            DEBUG_PRINT("Server->accept callback ended, this= " << connection.get() << " use_count= " << connection.use_count());
        }
    );
    DEBUG_PRINT("Server->accept method ended, this= " << connection.get() << " use_count= " << connection.use_count());
}
