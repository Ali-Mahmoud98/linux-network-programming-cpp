#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "../interface/ISocket.hpp"

class Socket : public ISocket
{
public:
    Socket();
    // explicit is a keyword that prevents a constructor from being used
    // for implicit type conversions.
    ~Socket() override;

    Socket(const Socket&) = delete; // delete copy constructor to prevent copying of Socket instances
    Socket& operator=(const Socket&) = delete; // delete copy assignment operator to prevent copying of Socket instances

    Socket(Socket&& other) noexcept; // move constructor
    Socket& operator=(Socket&& other) noexcept; // move assignment operator

    // methods for socket operations
    void bind(unsigned short port);
    void listen(int backlog);
    Socket accept();
    void connect(const char* ip, unsigned short port);
    void send(const void* data, std::size_t size) override;
    int receive(void* buffer, std::size_t size) override;

private:
    int m_fd{-1}; // default member initializer

    explicit Socket(int fd);
};

#endif // SOCKET_HPP
