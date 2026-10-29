#pragma once

#include <asio.hpp>
#include <cstdint>
#include <memory>
#include <unordered_set>

#include "netserver/ports/IMessageEndpoint.hpp"

namespace netserver {

class TcpSession;

class TcpServer : public IMessageEndpoint {
public:
    TcpServer(asio::io_context& io, std::uint16_t port);
    ~TcpServer() override = default;

    void start(InboundMessageHandler on_message) override;
    void stop() override;

    std::size_t active_sessions_count() const;

private:
    void accept();
    void handle_session_close(std::shared_ptr<TcpSession> session);

private:
    asio::io_context& m_io;
    asio::ip::tcp::acceptor m_acceptor;
    InboundMessageHandler m_on_message;
    std::unordered_set<std::shared_ptr<TcpSession>> m_sessions;
};

} // namespace netserver
