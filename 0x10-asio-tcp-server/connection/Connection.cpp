#include "Connection.hpp"

#include <iostream>

#include "../debug/debug.h"

Connection::Connection(asio::io_context& io)
    : m_socket(io)
{
    DEBUG_PRINT("Connection created, this= " << this);
}

asio::ip::tcp::socket& Connection::socket()
{
    return m_socket;
}

void Connection::start()
{
    DEBUG_PRINT("Connection starting, this= " << this);
    read();
}

void Connection::read()
{
    DEBUG_PRINT("Connection->read method started, this= " << this);
    m_socket.async_read_some(
        asio::buffer(m_buffer),
        [this](const asio::error_code& error,
               std::size_t bytes_read)
        {
            if (error)
            {
                std::cout << "Read error: "
                          << error.message()
                          << '\n';
                return;
            }

            std::cout << "Received "
                      << bytes_read
                      << " bytes\n";

            write(bytes_read);
        }
    );
    DEBUG_PRINT("Connection->read method ended, this= " << this);
}

void Connection::write(std::size_t bytes_read)
{
    DEBUG_PRINT("Connection->write method started, this= " << this);
    asio::async_write(
        m_socket,
        asio::buffer(m_buffer.data(), bytes_read),
        [this](const asio::error_code& error,
               std::size_t bytes_written)
        {
            if (error)
            {
                std::cout << "Write error: "
                          << error.message()
                          << '\n';
                return;
            }

            std::cout << "Sent "
                      << bytes_written
                      << " bytes\n";

            read();
        }
    );
    DEBUG_PRINT("Connection->write method ended, this= " << this);
}
