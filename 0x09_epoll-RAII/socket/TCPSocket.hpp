#pragma once

#include "ISocket.hpp"

class TcpSocket : public ISocket
{
public:
    explicit TcpSocket(int fd);

    ~TcpSocket() override;

    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;

    TcpSocket(TcpSocket&& other) noexcept;
    TcpSocket& operator=(TcpSocket&& other) noexcept;

    int fd() const override;

    ssize_t sendData(
        const void* data,
        std::size_t size
    ) override;

    ssize_t receiveData(
        void* buffer,
        std::size_t size
    ) override;

    void closeSocket() override;

private:
    int m_fd{-1};
};
