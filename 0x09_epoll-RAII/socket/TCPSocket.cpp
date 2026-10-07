#include "TCPSocket.hpp"

#include <sys/socket.h>
#include <unistd.h>

TcpSocket::TcpSocket(int fd)
    : m_fd(fd)
{
}

TcpSocket::~TcpSocket()
{
    closeSocket();
}

TcpSocket::TcpSocket(TcpSocket&& other) noexcept
    : m_fd(other.m_fd)
{
    other.m_fd = -1;
}

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept
{
    if (this != &other)
    {
        closeSocket();

        m_fd = other.m_fd;
        other.m_fd = -1;
    }

    return *this;
}

int TcpSocket::fd() const
{
    return m_fd;
}

ssize_t TcpSocket::sendData(
    const void* data,
    std::size_t size)
{
    return ::send(
        m_fd,
        data,
        size,
        0
    );
}

ssize_t TcpSocket::receiveData(
    void* buffer,
    std::size_t size)
{
    return ::recv(
        m_fd,
        buffer,
        size,
        0
    );
}

void TcpSocket::closeSocket()
{
    if (m_fd != -1)
    {
        ::close(m_fd);
        m_fd = -1;
    }
}
