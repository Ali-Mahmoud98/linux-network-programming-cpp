#pragma once

#include "netserver/common/debug.hpp"
#include "netserver/ports/InboundMessage.hpp"

namespace netserver {

class EchoHandler {
public:
    void on_message(const InboundMessage& inbound) {
        DEBUG_PRINT("[EchoHandler] Processing message (" << inbound.message.payload.size()
                    << " bytes) from " << inbound.peer.address << ":" << inbound.peer.port);

        if (inbound.reply_channel) {
            // We respond with the same message via the
            // abstract response channel (whether TCP or UDP in the future)
            inbound.reply_channel->send(inbound.message);
        }
    }
};

} // namespace netserver
