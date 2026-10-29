#pragma once

#include <asio.hpp>
#include <array>
#include <deque>
#include <functional>
#include <memory>
#include <string>

#include "netserver/core/PeerInfo.hpp"
#include "netserver/ports/InboundMessageHandler.hpp"
#include "netserver/ports/IReplyChannel.hpp"

namespace netserver {

class TcpSession : public std::enable_shared_from_this<TcpSession>,
                   public IReplyChannel
{
public:
    using CloseCallback = std::function<void(std::shared_ptr<TcpSession>)>;
    using MessageHandler = InboundMessageHandler;

    constexpr static std::size_t kReadBufferSize = 1024;
    constexpr static std::size_t kMaxWriteQueueSize = 64;
    constexpr static std::size_t kMaxMessageSize = 64 * 1024;
    constexpr static std::size_t kMaxWriteQueueBytes = 1024 * 1024;

    explicit TcpSession(asio::io_context& io,
                        MessageHandler on_message,
                        CloseCallback on_close);
    ~TcpSession() override;

    TcpSession(const TcpSession&) = delete;
    TcpSession& operator=(const TcpSession&) = delete;

    asio::ip::tcp::socket& socket();

    void start();
    void stop();

    // تنفيذ واجهة IReplyChannel
    void send(Message message) override;

private:
    void read();
    void write();
    void handle_error(const char* operation, const asio::error_code& error);

private:
    asio::ip::tcp::socket m_socket;
    PeerInfo m_peer;
    std::array<char, kReadBufferSize> m_read_buffer{};
    std::deque<std::string> m_write_queue;
    std::size_t m_write_queue_bytes{0};
    MessageHandler m_on_message;
    CloseCallback m_on_close;
    bool m_stopped{false};
};

} // namespace netserver
