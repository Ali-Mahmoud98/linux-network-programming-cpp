#pragma once

#include <asio.hpp>
#include <array>
#include <memory>
#include <queue>

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    using CloseCallback = std::function<void(std::shared_ptr<Connection>)>;

    constexpr static std::size_t kReadBufferSize = 1024;
    constexpr static std::size_t kMaxWriteQueueSize = 64;

    explicit Connection(asio::io_context& io, CloseCallback on_close);
    ~Connection();

    // Copying is prevented because the connection represents a single socket within the kernel.
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    asio::ip::tcp::socket& socket();

    void start();
    void stop();

    void send(std::string message);

private:
    void read();
    void write();
    void handle_error(const char* operation, const asio::error_code& error);

private:
    asio::ip::tcp::socket m_socket;
    std::array<char, kReadBufferSize> m_read_buffer;
    std::deque<std::string> m_write_queue;
    CloseCallback m_on_close;
    bool m_stopped{false};
};
