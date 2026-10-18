#include "Server.hpp"

#include "Connection.hpp"
#include <iostream>

#include "debug.hpp"

Server::Server(asio::io_context& io, unsigned short port)
    : m_io(io),
      m_acceptor(io,
        asio::ip::tcp::endpoint(
            asio::ip::tcp::v4(),
            port
        )
    )
{
    DEBUG_PRINT("Server created, this(Server)= " << this);
}

void Server::start()
{
    DEBUG_PRINT("Server starting, this(Server)= " << this);
    accept();
}

void Server::stop()
{
    DEBUG_PRINT("Server stopping, this(Server)= " << this << ", closing acceptor and all active connections...");

    asio::error_code ignored_ec;
    m_acceptor.close(ignored_ec);

    // Taking a copy of the set because connection->stop() will remove the element from m_connections
    // If we iterated directly over m_connections and called erase inside the loop, it would invalidate the iterator.
    auto connections_copy = m_connections;
    for (auto& conn : connections_copy)
    {
        conn->stop();
    }
    m_connections.clear();
}

std::size_t Server::active_connections_count() const
{
    return m_connections.size();
}

void Server::handle_connection_close(std::shared_ptr<Connection> connection)
{
    m_connections.erase(connection);
    DEBUG_PRINT("Connection removed. Active connections: " << m_connections.size());
}

void Server::accept()
{
    auto connection = std::make_shared<Connection>(
        m_io,
        [this](std::shared_ptr<Connection> closed_connection)
        {
            handle_connection_close(closed_connection);
        } 
    );

    DEBUG_PRINT("Server->accept method started, conn= " << connection.get()
                << " use_count= " << connection.use_count());

    m_acceptor.async_accept(
        connection->socket(),
        [this, connection](const asio::error_code& ec)
        {
            DEBUG_PRINT("Server->accept callback, conn= " << connection.get() << " use_count= " << connection.use_count());
            if (ec)
            {
                if (ec == asio::error::operation_aborted) {
                    DEBUG_PRINT("Acceptor closed, stopping accept loop.");
                    return; // stop the accept loop if the acceptor is closed
                }

                DEBUG_PRINT_ERR("Accept error: " << ec.message());
            }
            else
            {
                m_connections.insert(connection); // use_count will increase as the shared_ptr is now also held by the set
                DEBUG_PRINT("Server->accept: Client connection added to active connections. conn= " << connection.get() << " use_count= " << connection.use_count());
                DEBUG_PRINT("Client connected, active connections: " << m_connections.size());
                connection->start();
            }

            if (m_acceptor.is_open())
            {
                accept();
            }

            DEBUG_PRINT("Server->accept callback ended, conn= " << connection.get()
                        << " use_count= " << connection.use_count()); // when the accept callback ends the use count
                        // decreases as the shared_ptr goes out of scope - if the the printed number is 3 it means
                        // when this lambda goes out of scope, the use count will decrease accordingly to be 2.
        }
    );
}
