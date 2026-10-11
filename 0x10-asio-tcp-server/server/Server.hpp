#pragma once

#include <asio.hpp>

class Server
{
public:
    Server(asio::io_context& io, unsigned short port);

    void start();

private:
    void accept();

private:
    asio::io_context& m_io;
    asio::ip::tcp::acceptor m_acceptor;
};
