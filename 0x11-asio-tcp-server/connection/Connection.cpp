#include "Connection.hpp"

#include <iostream>

Connection::Connection(asio::io_context& io)
    : m_socket(io)
{
}

asio::ip::tcp::socket& Connection::socket()
{
    return m_socket;
}

void Connection::start()
{
    read();
}

void Connection::read()
{
    auto self = shared_from_this();

    m_socket.async_read_some(
        asio::buffer(m_buffer),
        [this, self](const asio::error_code& error,
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
}

void Connection::write(std::size_t bytes_read)
{
    auto self = shared_from_this();

    asio::async_write(
        m_socket,
        asio::buffer(m_buffer.data(), bytes_read),
        [this, self](const asio::error_code& error,
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
}
