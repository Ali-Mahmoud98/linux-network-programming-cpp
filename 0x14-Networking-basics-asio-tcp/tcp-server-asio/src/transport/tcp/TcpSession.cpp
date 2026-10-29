#include "netserver/transport/tcp/TcpSession.hpp"
#include "netserver/common/debug.hpp"

namespace netserver {

TcpSession::TcpSession(asio::io_context& io,
                       MessageHandler on_message,
                       CloseCallback on_close)
    : m_socket(io),
      m_on_message(std::move(on_message)),
      m_on_close(std::move(on_close))
{
    DEBUG_PRINT("[Constructor] TcpSession created, this=" << this);
}

TcpSession::~TcpSession()
{
    DEBUG_PRINT("[Destructor] TcpSession destroyed, this=" << this);
}

asio::ip::tcp::socket& TcpSession::socket()
{
    return m_socket;
}

void TcpSession::start()
{
    DEBUG_PRINT("Start TcpSession::start() method, this=" << this);

    // Extract client information (IP & Port) once at the start of the session
    asio::error_code ec;
    auto endpoint = m_socket.remote_endpoint(ec);
    if (!ec) {
        m_peer.transport = TransportType::Tcp;
        m_peer.address = endpoint.address().to_string();
        m_peer.port = endpoint.port();
    }

    // Test the Write Queue with welcome messages (we will move or remove them in step 03 with the Binary protocol)
    send(Message{"Welcome to Asio TCP Server!\n"});
    send(Message{"Type anything and I will echo it back!\n"});

    read();
    DEBUG_PRINT("End TcpSession::start() method");
}

void TcpSession::send(Message message)
{
    DEBUG_PRINT("Start TcpSession::send() method");
    if (m_stopped || message.payload.empty()) {
        return;
    }

    const std::size_t size = message.payload.size();
    if (size > kMaxMessageSize) {
        DEBUG_PRINT_ERR("Message too large (" << size << " bytes)! Closing session, this=" << this);
        stop();
        return;
    }

    // Backpressure: bound both the entry count and total queued bytes
    if (m_write_queue.size() >= kMaxWriteQueueSize ||
        m_write_queue_bytes + size > kMaxWriteQueueBytes) {
        DEBUG_PRINT_ERR("Write queue overflow (" << m_write_queue.size() << " messages, "
                        << m_write_queue_bytes << " bytes)! Disconnecting slow client, this=" << this);
        stop();
        return;
    }

    const bool write_in_progress = !m_write_queue.empty();
    m_write_queue.push_back(std::move(message.payload));
    m_write_queue_bytes += size;

    DEBUG_PRINT("Queued message, queue_size=" << m_write_queue.size()
                << " write_in_progress=" << write_in_progress
                << " this=" << this);

    if (!write_in_progress) {
        write();
    }
    DEBUG_PRINT("End TcpSession::send() method");
}

void TcpSession::stop()
{
    DEBUG_PRINT("Start TcpSession::stop() method");
    if (m_stopped) {
        return;
    }

    m_stopped = true;
    DEBUG_PRINT("TcpSession stopping, this=" << this);

    asio::error_code ignored_ec;
    m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
    m_socket.close(ignored_ec);

    if (m_on_close) {
        auto callback = std::move(m_on_close);
        callback(shared_from_this());
    }

    DEBUG_PRINT("End TcpSession::stop() method");
}

void TcpSession::read()
{
    DEBUG_PRINT("Start TcpSession::read() method");
    auto self = shared_from_this();
    DEBUG_PRINT("TcpSession->read started, this=" << this << " use_count=" << self.use_count());

    m_socket.async_read_some(
        asio::buffer(m_read_buffer),
        [this, self](const asio::error_code& error, std::size_t bytes_read)
        {
            DEBUG_PRINT("TcpSession Read callback, this=" << this << " use_count=" << self.use_count());
            if (error) {
                handle_error("Read", error);
                return;
            }

            DEBUG_PRINT("Received " << bytes_read << " raw bytes");

            // In step 03: this is exactly where we will feed the bytes into the LengthPrefixFramer!
            // For now, we construct the InboundMessage directly and pass it to the application layer (EchoHandler)
            if (m_on_message) {
                InboundMessage inbound{
                    Message{std::string(m_read_buffer.data(), bytes_read)},
                    m_peer,
                    self // The TcpSession itself is the IReplyChannel!
                };
                m_on_message(inbound);
            }

            if (!m_stopped) {
                read();
            }
        }
    );
    DEBUG_PRINT("End TcpSession::read() method");
}

void TcpSession::write()
{
    DEBUG_PRINT("Start TcpSession::write() method");
    auto self = shared_from_this();
    DEBUG_PRINT("TcpSession->write started, queue_size=" << m_write_queue.size()
                << " msg_len=" << m_write_queue.front().size()
                << " this=" << this << " use_count=" << self.use_count());

    asio::async_write(
        m_socket,
        asio::buffer(m_write_queue.front()),
        [this, self](const asio::error_code& error, std::size_t bytes_written)
        {
            DEBUG_PRINT("TcpSession Write callback, bytes_written=" << bytes_written
                        << " this=" << this << " use_count=" << self.use_count());
            if (error) {
                handle_error("Write", error);
                return;
            }

            DEBUG_PRINT("Sent " << bytes_written << " bytes");

            m_write_queue_bytes -= m_write_queue.front().size();
            m_write_queue.pop_front();

            if (!m_write_queue.empty() && !m_stopped) {
                write();
            }
        }
    );
    DEBUG_PRINT("End TcpSession::write() method");
}

void TcpSession::handle_error(const char* operation, const asio::error_code& error)
{
    DEBUG_PRINT("Start TcpSession::handle_error() method");
    if (error == asio::error::operation_aborted) {
        DEBUG_PRINT("[" << operation << "] Operation cancelled locally (Socket closed).");
    } else if (error == asio::error::eof) {
        DEBUG_PRINT("[" << operation << "] Client disconnected gracefully (EOF).");
    } else if (error == asio::error::connection_reset) {
        DEBUG_PRINT_ERR("[" << operation << "] Client connection reset (RST).");
    } else {
        DEBUG_PRINT_ERR("[" << operation << "] Unexpected error: " << error.message());
    }

    stop();
    DEBUG_PRINT("End TcpSession::handle_error() method");
}

} // namespace netserver
