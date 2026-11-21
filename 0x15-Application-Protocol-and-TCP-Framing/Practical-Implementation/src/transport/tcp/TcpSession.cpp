#include "netserver/transport/tcp/TcpSession.hpp"
#include "netserver/common/debug.hpp"

namespace netserver {

TcpSession::TcpSession(asio::io_context& io,
                       std::unique_ptr<IStreamProtocol> protocol,
                       MessageHandler on_message,
                       ConnectionOpenedHandler on_open,
                       CloseCallback on_close)
    : m_socket(io),
      m_protocol(std::move(protocol)),
      m_on_message(std::move(on_message)),
      m_on_open(std::move(on_open)),
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

    notify_open_if_ready();

    // The application may have closed the session from on_open
    if (!m_stopped) {
        read();
    }
    DEBUG_PRINT("End TcpSession::start() method");
}

void TcpSession::notify_open_if_ready()
{
    if (m_open_notified || m_stopped || !m_protocol->is_open()) {
        return;
    }

    m_open_notified = true;
    if (m_on_open) {
        m_on_open(m_peer, shared_from_this());
    }
}

void TcpSession::send(Message message)
{
    DEBUG_PRINT("Start TcpSession::send() method");
    if (m_stopped || !m_protocol->is_open()) {
        return;
    }

    enqueue_raw(m_protocol->encode(message));
    DEBUG_PRINT("End TcpSession::send() method");
}

void TcpSession::enqueue_raw(std::string bytes)
{
    DEBUG_PRINT("Start TcpSession::enqueue_raw() method");
    if (m_stopped || bytes.empty()) {
        return;
    }

    const std::size_t size = bytes.size();
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
    m_write_queue.push_back(std::move(bytes));
    m_write_queue_bytes += size;

    DEBUG_PRINT("Queued message, queue_size=" << m_write_queue.size()
                << " write_in_progress=" << write_in_progress
                << " this=" << this);

    if (!write_in_progress) {
        write();
    }
    DEBUG_PRINT("End TcpSession::enqueue_raw() method");
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

            ProtocolEvents events = m_protocol->on_bytes(m_read_buffer.data(), bytes_read);

            if (!events.outbound.empty()) {
                enqueue_raw(std::move(events.outbound));
            }

            // A handshake may have just completed inside on_bytes()
            notify_open_if_ready();

            for (auto& message : events.messages) {
                if (m_stopped) {
                    return;
                }
                if (m_on_message) {
                    InboundMessage inbound{
                        std::move(message),
                        m_peer,
                        self // The TcpSession itself is the IReplyChannel!
                    };
                    m_on_message(inbound);
                }
            }

            if (m_stopped) {
                return;
            }

            if (events.error) {
                DEBUG_PRINT_ERR("Protocol error! Closing session, this=" << this);
                stop();
                return;
            }

            if (events.close_after_flush) {
                m_close_after_flush = true;
                if (m_write_queue.empty()) {
                    stop();
                }
                return;
            }

            read();
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

            if (m_stopped) {
                return;
            }

            if (!m_write_queue.empty()) {
                write();
            } else if (m_close_after_flush) {
                stop();
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
