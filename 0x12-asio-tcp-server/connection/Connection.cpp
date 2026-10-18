#include "Connection.hpp"

#include <iostream>


#include "debug.hpp"

Connection::Connection(asio::io_context& io, CloseCallback on_close)
    : m_socket(io), m_on_close(std::move(on_close))
{
    DEBUG_PRINT("Connection created, this(Connection)= " << this);
}

Connection::~Connection()
{
    DEBUG_PRINT("Connection destroyed, this(Connection)= " << this);
}

asio::ip::tcp::socket& Connection::socket()
{
    return m_socket;
}

void Connection::start()
{
    DEBUG_PRINT("Connection started, this(Connection)= " << this);
    read();
}

void Connection::stop()
{
    // Protection against repeated execution if stop() is called from multiple places
    if (m_stopped) {
        return;
    }

    m_stopped = true;

    DEBUG_PRINT("Connection stopping, this(Connection)= " << this);

    asio::error_code ignored_ec;
    // Closing the socket cancels any pending async operations (returning operation_aborted)
    m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
    m_socket.close(ignored_ec);

    // The server removes this connection from the list of active connections.
    if (m_on_close) {
        // We move the callback to a local variable and clear the member to prevent any reference cycle.
        auto callback = std::move(m_on_close);
        callback(shared_from_this());
    }
}

void Connection::read()
{
    auto self = shared_from_this();
    DEBUG_PRINT("Connection->read method started, this(Connection)= " << this << " use_count= " << self.use_count());

    m_socket.async_read_some(
        asio::buffer(m_buffer),
        [this, self](const asio::error_code& error,
               std::size_t bytes_read)
        {
            DEBUG_PRINT("Connection Read callback, this(Connection)= " << this << " use_count= " << self.use_count());
            if (error)
            {
                handle_error("Read", error);
                return;
            }

            DEBUG_PRINT("Received " << bytes_read << " bytes");

            write(bytes_read);
        }
    );
    DEBUG_PRINT("Connection->read method ended, this(Connection)= " << this << " use_count= " << self.use_count());
}

void Connection::write(std::size_t bytes_read)
{
    auto self = shared_from_this();
    DEBUG_PRINT("Connection->write method started, this(Connection)= " << this << " use_count= " << self.use_count());

    asio::async_write(
        m_socket,
        asio::buffer(m_buffer.data(), bytes_read),
        [this, self](const asio::error_code& error,
               std::size_t bytes_written)
        {
            DEBUG_PRINT("Connection Write callback, this(Connection)= " << this << " use_count= " << self.use_count());
            if (error)
            {
                handle_error("Write", error);
                return;
            }

            DEBUG_PRINT("Sent " << bytes_written << " bytes");

            read();
        }
    );
    DEBUG_PRINT("Connection->write method ended, this(Connection)= " << this << " use_count= " << self.use_count());
}

void Connection::handle_error(const char* operation, const asio::error_code& error)
{
    if (error == asio::error::operation_aborted)
    {
        // The operation was cancelled because we called stop() locally.
        DEBUG_PRINT("[" << operation << "] Operation cancelled locally (Socket closed).");
    }
    else if (error == asio::error::eof)
    {
        // The client closed the connection gracefully (FIN)
        DEBUG_PRINT("[" << operation << "] Client disconnected gracefully (EOF).");
    }
    else if (error == asio::error::connection_reset)
    {
        // The client closed the connection abruptly (RST)
        DEBUG_PRINT("[" << operation << "] Client connection reset (RST).");
    }
    else
    {
        DEBUG_PRINT("[" << operation << "] Unexpected error: " << error.message());
    }

    // In all stop cases, ensure the connection is closed and removed from the server.
    stop();
}
