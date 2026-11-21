#pragma once

#include "netserver/protocols/IStreamProtocol.hpp"

namespace netserver {

// No framing: every chunk read from the stream is delivered as one message.
class RawStreamProtocol : public IStreamProtocol {
public:
    ProtocolEvents on_bytes(const char* data, std::size_t size) override {
        ProtocolEvents events;
        events.messages.push_back(Message{std::string(data, size)});
        return events;
    }

    std::string encode(const Message& message) override { return message.payload; }

    bool is_open() const override { return true; }
};

} // namespace netserver
