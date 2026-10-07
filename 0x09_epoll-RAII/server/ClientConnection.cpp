#include "ClientConnection.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>

ClientConnection::ClientConnection(int fd)
    : m_socket(fd)
{
}

int ClientConnection::fd() const
{
    return m_socket.fd();
}

TcpSocket& ClientConnection::socket()
{
    return m_socket;
}

void ClientConnection::appendOutput(
    const char* data,
    std::size_t size)
{
    m_outputBuffer.append(data, size);
}

bool ClientConnection::hasPendingOutput() const
{
    return m_outputOffset < m_outputBuffer.size();
}

bool ClientConnection::writeAvailable()
{
    while (m_outputOffset < m_outputBuffer.size())
    {
        const char* data =
            m_outputBuffer.data()
            + m_outputOffset;

        std::size_t remaining =
            m_outputBuffer.size()
            - m_outputOffset;

        ssize_t bytesSent =
            m_socket.sendData(
                data,
                remaining
            );

        if (bytesSent > 0)
        {
            m_outputOffset +=
                static_cast<std::size_t>(bytesSent);

            std::cout
                << "[Client "
                << fd()
                << "] sent "
                << bytesSent
                << " bytes, remaining = "
                << m_outputBuffer.size()
                   - m_outputOffset
                << '\n';

            continue;
        }

        if (bytesSent == -1 &&
            (errno == EAGAIN ||
             errno == EWOULDBLOCK))
        {
            std::cout
                << "[Client "
                << fd()
                << "] send would block\n";

            return true;
        }

        std::cerr
            << "[Client "
            << fd()
            << "] send failed: "
            << std::strerror(errno)
            << '\n';

        return false;
    }

    // Everything was sent.

    m_outputBuffer.clear();
    m_outputOffset = 0;

    std::cout
        << "[Client "
        << fd()
        << "] output buffer drained\n";

    return true;
}
