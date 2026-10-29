#pragma once

#include <memory>
#include "netserver/core/Message.hpp"
#include "netserver/core/PeerInfo.hpp"
#include "netserver/ports/IReplyChannel.hpp"

namespace netserver {

// In networking, the term "inbound" refers to traffic entering a
// server (as opposed to "outbound," which refers to traffic leaving
// a server). Therefore, "InboundMessage" signifies an incoming
// message, including all its components.
struct InboundMessage {
    Message message;
    PeerInfo peer;
    std::shared_ptr<IReplyChannel> reply_channel;
};

} // namespace netserver
