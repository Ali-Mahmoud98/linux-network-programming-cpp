#pragma once

#include <asio.hpp>
#include <unordered_set>
#include "Connection.hpp"

class Server
{
public:
    Server(asio::io_context& io, unsigned short port);

    void start();
    void stop();

    std::size_t active_connections_count() const;

private:
    void accept();
    void handle_connection_close(std::shared_ptr<Connection> connection);

private:
    asio::io_context& m_io;
    asio::ip::tcp::acceptor m_acceptor;
    std::unordered_set<std::shared_ptr<Connection>> m_connections;
};
