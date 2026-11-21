#pragma once

#include <cstddef>
#include <optional>
#include <string>

#include "netserver/core/Message.hpp"

namespace netserver {

// Sans-I/O protocol over datagrams (UDP). Each datagram is self-contained, so one instance serves the whole server.
class IDatagramProtocol {
public:
    virtual ~IDatagramProtocol() = default;

    // std::nullopt means the datagram is invalid and should be dropped.
    virtual std::optional<Message> decode(const char* data, std::size_t size) = 0;
    virtual std::string encode(const Message& message) = 0;
};

} // namespace netserver
