#include "Connection.hpp"

#include <iostream>


#include "../debug/debug.h"

Connection::Connection(asio::io_context& io)
    : m_socket(io)
{
    DEBUG_PRINT("Connection created, this= " << this);
}

Connection::~Connection()
{
    DEBUG_PRINT("Connection destroyed, this= " << this);
}

asio::ip::tcp::socket& Connection::socket()
{
    return m_socket;
}

void Connection::start()
{
    DEBUG_PRINT("Connection started, this= " << this);
    read();
}

void Connection::read()
{
    auto self = shared_from_this();
    DEBUG_PRINT("Connection->read method started, this= " << this << " use_count= " << self.use_count());

    m_socket.async_read_some(
        asio::buffer(m_buffer),
        [this, self](const asio::error_code& error,
               std::size_t bytes_read)
        {
            DEBUG_PRINT("Connection Read callback, this= " << this << " use_count= " << self.use_count());
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
    DEBUG_PRINT("Connection->read method ended, this= " << this << " use_count= " << self.use_count());
}

void Connection::write(std::size_t bytes_read)
{
    auto self = shared_from_this();
    DEBUG_PRINT("Connection->write method started, this= " << this << " use_count= " << self.use_count());

    asio::async_write(
        m_socket,
        asio::buffer(m_buffer.data(), bytes_read),
        [this, self](const asio::error_code& error,
               std::size_t bytes_written)
        {
            DEBUG_PRINT("Connection Write callback, this= " << this << " use_count= " << self.use_count());
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
}
