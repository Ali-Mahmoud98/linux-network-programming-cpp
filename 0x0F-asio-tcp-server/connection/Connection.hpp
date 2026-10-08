#pragma once

#include <asio.hpp>
#include <array>

class Connection
{
public:
    explicit Connection(asio::io_context& io);

    asio::ip::tcp::socket& socket();

    void start();

private:
    void read();

    void write(std::size_t bytes_read);

private:
    asio::ip::tcp::socket m_socket;
    std::array<char, 1024> m_buffer;
};
