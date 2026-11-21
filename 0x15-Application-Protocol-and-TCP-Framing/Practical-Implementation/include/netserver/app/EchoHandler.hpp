#pragma once

#include <memory>
#include "netserver/common/debug.hpp"
#include "netserver/core/PeerInfo.hpp"
#include "netserver/ports/InboundMessage.hpp"
#include "netserver/ports/IReplyChannel.hpp"

namespace netserver {

class EchoHandler {
public:
    void on_open([[maybe_unused]] const PeerInfo& peer, const std::shared_ptr<IReplyChannel>& channel) {
        DEBUG_PRINT("[EchoHandler] New connection from " << peer.address << ":" << peer.port);

        channel->send(Message{"Welcome to Asio TCP Server!\n"});
        channel->send(Message{"Type anything and I will echo it back!\n"});
    }

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
