#include "Connection.hpp"

#include <iostream>


#include "debug.hpp"

Connection::Connection(asio::io_context& io, CloseCallback on_close)
    : m_socket(io), m_on_close(std::move(on_close))
{
    DEBUG_PRINT("[Constructor] Connection created, this(Connection)= " << this);
}

Connection::~Connection()
{
    DEBUG_PRINT("[Destructor] Connection destroyed, this(Connection)= " << this);
}

asio::ip::tcp::socket& Connection::socket()
{
    DEBUG_PRINT("Start Connection::socket() method");
    DEBUG_PRINT("End Connection::socket() method");
    return m_socket;
}

void Connection::start()
{
    DEBUG_PRINT("Start Connection::start() method");
    DEBUG_PRINT("Connection started, this(Connection)= " << this);

    // We test the Write Queue by sending two messages in quick succession the moment the client connects!
    send("Welcome to Asio TCP Server!\n");
    send("Type anything and I will echo it back!\n");

    read();
    DEBUG_PRINT("End Connection::start() method");
}

void Connection::send(std::string message) {
    DEBUG_PRINT("Start Connection::send() method");
    if (m_stopped || message.empty()) {
        return;
    }

    // Backpressure check: Protecting memory from a slow client
    if (m_write_queue.size() >= kMaxWriteQueueSize) {
        DEBUG_PRINT_ERR("Write queue overflow (" << m_write_queue.size()
                        << " messages)! Disconnecting slow client, this=" << this);
        stop();
        return;
    }

    // If the queue is empty, it means there is no async_write currently in progress.
    const bool write_in_progress = !m_write_queue.empty();
    m_write_queue.push_back(std::move(message));

    DEBUG_PRINT("Queued message, queue_size=" << m_write_queue.size()
                << " write_in_progress=" << write_in_progress
                << " this=" << this);
    
    if (!write_in_progress) {
        write();
    }
    DEBUG_PRINT("End Connection::send() method");
}

void Connection::stop()
{
    DEBUG_PRINT("Start Connection::stop() method");
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

    DEBUG_PRINT("End Connection::stop() method");
}

void Connection::read()
{
    DEBUG_PRINT("Start Connection::read() method");
    auto self = shared_from_this();
    DEBUG_PRINT("Connection->read method started, this(Connection)= " << this << " use_count= " << self.use_count());

    m_socket.async_read_some(
        asio::buffer(m_read_buffer),
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

            send(std::string(m_read_buffer.data(), bytes_read));

            read();
        }
    );
    DEBUG_PRINT("End Connection::read() method");
    DEBUG_PRINT("Connection->read method ended, this(Connection)= " << this << " use_count= " << self.use_count());
}

void Connection::write()
{
    DEBUG_PRINT("Start Connection::write() method");
    auto self = shared_from_this();
    DEBUG_PRINT("Connection->write method started, queue_size= " << m_write_queue.size()
        << " msg_len= " << m_write_queue.front().size()
        << " this(Connection)= " << this << " use_count= " << self.use_count());

    asio::async_write(
        m_socket,
        asio::buffer(m_write_queue.front()),
        [this, self](const asio::error_code& error,
               std::size_t bytes_written)
        {
            DEBUG_PRINT("Connection Write callback, bytes_written=" << bytes_written
                << " this(Connection)= " << this << " use_count= " << self.use_count());
            if (error)
            {
                handle_error("Write", error);
                return;
            }

            DEBUG_PRINT("Sent " << bytes_written << " bytes");

            // The message has been fully delivered to the kernel; remove it from the queue.
            m_write_queue.pop_front();

            if (!m_write_queue.empty() && !m_stopped)
            {
                write();
            }
        }
    );
    DEBUG_PRINT("Connection->write method ended, this(Connection)= " << this << " use_count= " << self.use_count());
    DEBUG_PRINT("End Connection::write() method");
}

void Connection::handle_error(const char* operation, const asio::error_code& error)
{
    DEBUG_PRINT("Start Connection::handle_error() method");
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
    DEBUG_PRINT("End Connection::handle_error() method");
}
