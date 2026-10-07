#pragma once

#include "TCPSocket.hpp"

#include <cstddef>
#include <string>

class ClientConnection
{
public:
    explicit ClientConnection(int fd);

    int fd() const;

    TcpSocket& socket();

    void appendOutput(
        const char* data,
        std::size_t size
    );

    bool hasPendingOutput() const;

    bool writeAvailable();

private:
    TcpSocket m_socket;

    std::string m_outputBuffer;

    std::size_t m_outputOffset{0};
};
