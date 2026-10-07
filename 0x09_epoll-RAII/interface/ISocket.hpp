#pragma once

#include <cstddef>
#include <sys/types.h>

class ISocket
{
public:
    virtual ~ISocket() = default;

    virtual int fd() const = 0;

    virtual ssize_t sendData(
        const void* data,
        std::size_t size
    ) = 0;

    virtual ssize_t receiveData(
        void* buffer,
        std::size_t size
    ) = 0;

    virtual void closeSocket() = 0;
};
