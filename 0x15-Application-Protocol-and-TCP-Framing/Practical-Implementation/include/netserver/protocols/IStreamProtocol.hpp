#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "netserver/core/Message.hpp"

namespace netserver {

// What a stream protocol asks the transport to do after consuming some bytes.
struct ProtocolEvents {
    std::vector<Message> messages;   // complete application messages
    std::string outbound;            // protocol-level bytes: handshake reply, pong, close frame
    bool close_after_flush{false};
    bool error{false};
};

// Sans-I/O protocol over a byte stream (TCP, TLS). One instance per connection.
class IStreamProtocol {
public:
    virtual ~IStreamProtocol() = default;

    virtual ProtocolEvents on_bytes(const char* data, std::size_t size) = 0;
    virtual std::string encode(const Message& message) = 0;
    virtual bool is_open() const = 0;   // false until the handshake completes
};

using StreamProtocolFactory = std::function<std::unique_ptr<IStreamProtocol>()>;

} // namespace netserver
