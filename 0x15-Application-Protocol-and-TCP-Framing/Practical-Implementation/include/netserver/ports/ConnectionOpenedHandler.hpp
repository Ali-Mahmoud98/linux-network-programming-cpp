#pragma once

#include <functional>
#include <memory>
#include "netserver/core/PeerInfo.hpp"
#include "netserver/ports/IReplyChannel.hpp"

namespace netserver {

// Invoked once per new connection, before any inbound message; connectionless transports never call it.
using ConnectionOpenedHandler =
    std::function<void(const PeerInfo&, std::shared_ptr<IReplyChannel>)>;

} // namespace netserver
