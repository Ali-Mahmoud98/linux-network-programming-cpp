#ifndef ISOCKET_HPP
#define ISOCKET_HPP

#include <cstddef>

class ISocket
{
public:
    virtual ~ISocket() = default;

    virtual void send(const void* data, std::size_t size) = 0;

    virtual int receive(void* buffer, std::size_t size) = 0;
};

#endif // ISOCKET_HPP
