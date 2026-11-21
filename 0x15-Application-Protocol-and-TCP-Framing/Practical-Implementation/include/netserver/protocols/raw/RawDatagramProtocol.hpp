#pragma once

#include "netserver/protocols/IDatagramProtocol.hpp"

namespace netserver {

// Every datagram is delivered as one message, unchanged.
class RawDatagramProtocol : public IDatagramProtocol {
public:
    std::optional<Message> decode(const char* data, std::size_t size) override {
        return Message{std::string(data, size)};
    }

    std::string encode(const Message& message) override { return message.payload; }
};

} // namespace netserver
