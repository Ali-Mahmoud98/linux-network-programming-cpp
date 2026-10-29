#include "netserver/transport/tcp/TcpServer.hpp"
#include "netserver/transport/tcp/TcpSession.hpp"
#include "netserver/common/debug.hpp"

namespace netserver {

TcpServer::TcpServer(asio::io_context& io, std::uint16_t port)
    : m_io(io),
      m_acceptor(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
{
    DEBUG_PRINT("[Constructor] TcpServer created on port " << port << ", this=" << this);
}

void TcpServer::start(InboundMessageHandler on_message)
{
    DEBUG_PRINT("Start TcpServer::start() method, this=" << this);
    m_on_message = std::move(on_message);
    accept();
    DEBUG_PRINT("End TcpServer::start()");
}

void TcpServer::stop()
{
    DEBUG_PRINT("Start TcpServer::stop() method");
    DEBUG_PRINT("TcpServer stopping, this=" << this << ", closing acceptor and all active sessions...");

    asio::error_code ignored_ec;
    m_acceptor.close(ignored_ec);

    auto sessions_copy = m_sessions;
    for (auto& session : sessions_copy) {
        session->stop();
    }
    m_sessions.clear();
    DEBUG_PRINT("End TcpServer::stop() method");
}

std::size_t TcpServer::active_sessions_count() const
{
    return m_sessions.size();
}

void TcpServer::handle_session_close(std::shared_ptr<TcpSession> session)
{
    DEBUG_PRINT("Start TcpServer::handle_session_close() method");
    m_sessions.erase(session);
    DEBUG_PRINT("Session removed. Active sessions: " << m_sessions.size());
    DEBUG_PRINT("End TcpServer::handle_session_close() method");
}

void TcpServer::accept()
{
    DEBUG_PRINT("Start TcpServer::accept() method");
    auto session = std::make_shared<TcpSession>(
        m_io,
        m_on_message,
        [this](std::shared_ptr<TcpSession> closed_session) {
            handle_session_close(std::move(closed_session));
        }
    );

    DEBUG_PRINT("TcpServer->accept started, session=" << session.get()
                << " use_count=" << session.use_count());

    m_acceptor.async_accept(
        session->socket(),
        [this, session](const asio::error_code& ec)
        {
            DEBUG_PRINT("TcpServer->accept callback, session=" << session.get()
                        << " use_count=" << session.use_count());
            if (ec) {
                if (ec == asio::error::operation_aborted) {
                    DEBUG_PRINT("Acceptor closed, stopping accept loop.");
                    return;
                }
                DEBUG_PRINT_ERR("Accept error: " << ec.message());
            } else {
                m_sessions.insert(session);
                DEBUG_PRINT("Client connected! Active sessions: " << m_sessions.size()
                            << " session=" << session.get()
                            << " use_count=" << session.use_count());
                session->start();
            }

            if (m_acceptor.is_open()) {
                accept();
            }

            DEBUG_PRINT("TcpServer->accept callback ended, session=" << session.get()
                        << " use_count=" << session.use_count());
        }
    );
    DEBUG_PRINT("End TcpServer::accept() method");
}

} // namespace netserver
