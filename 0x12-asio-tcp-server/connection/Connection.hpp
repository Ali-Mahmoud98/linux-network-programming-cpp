#pragma once

#include <asio.hpp>
#include <array>
#include <memory>

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    using CloseCallback = std::function<void(std::shared_ptr<Connection>)>;

    explicit Connection(asio::io_context& io, CloseCallback on_close);
    ~Connection();

    // Copying is prevented because the connection represents a single socket within the kernel.
    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    asio::ip::tcp::socket& socket();

    void start();
    void stop();

private:
    void read();
    void write(std::size_t bytes_read);
    void handle_error(const char* operation, const asio::error_code& error);

private:
    asio::ip::tcp::socket m_socket;
    std::array<char, 1024> m_buffer;
    CloseCallback m_on_close;
    bool m_stopped{false};
};
