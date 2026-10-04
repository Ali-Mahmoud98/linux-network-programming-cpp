#include "Socket.hpp"

#include <sys/types.h> 
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdexcept>
#include <iostream>

Socket::Socket() {
    m_fd = ::socket(AF_INET, SOCK_STREAM, 0);

    if (m_fd == -1) {
        throw std::runtime_error("socket() failed");
    }
}

Socket::Socket(int fd) : m_fd(fd) {
    if (m_fd == -1) {
        throw std::runtime_error("socket() failed");
    }
}

Socket::~Socket() {
    if (m_fd != -1) {
        ::close(m_fd);
    }

    std::cout << "Destructor called." << std::endl;
}

Socket::Socket(Socket&& other) noexcept : m_fd(other.m_fd) {
    other.m_fd = -1;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        if (m_fd != -1) {
            ::close(m_fd);
        }
        m_fd = other.m_fd;
        other.m_fd = -1;
    }
    return *this;
}

// Starting socket operations methods
void Socket::bind(unsigned short port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (::bind(
            m_fd,
            reinterpret_cast<sockaddr*>(&addr),
            sizeof(addr)) == -1)
    {
        throw std::runtime_error("bind() failed");
    }
}

void Socket::listen(int backlog) {
    if (::listen(m_fd, backlog) == -1) {
        throw std::runtime_error("listen() failed");
    }
}

Socket Socket::accept() {
    int client_fd = ::accept(m_fd, nullptr, nullptr);

    if (client_fd == -1) {
        throw std::runtime_error("accept() failed");
    }
    return Socket(client_fd);
}

void Socket::connect(const char* ip, unsigned short port) {
    sockaddr_in address{};
    
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (inet_pton(
            AF_INET,
            ip,
            &address.sin_addr) != 1)
    {
        throw std::runtime_error("invalid IP address");
    }

    if (::connect(
            m_fd,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) == -1)
    {
        throw std::runtime_error("connect() failed");
    }
}

void Socket::send(const void* data, size_t size) {
    ssize_t result = ::send(m_fd, data, size, 0);

    if (result == -1) {
        throw std::runtime_error("send() failed");
    }
}

int Socket::receive(void* buffer, size_t size) {
    int result = ::recv(m_fd, buffer, size, 0);

    if (result == -1) {
        throw std::runtime_error("receive() failed");
    }
    return result;
}
