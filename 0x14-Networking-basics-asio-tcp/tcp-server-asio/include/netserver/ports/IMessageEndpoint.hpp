#pragma once

#include "netserver/ports/InboundMessageHandler.hpp"

namespace netserver {

// Lifecycle control of a transport (start/stop); not the application boundary.
class IMessageEndpoint {
public:
    virtual ~IMessageEndpoint() = default;

    virtual void start(InboundMessageHandler on_message) = 0;
    virtual void stop() = 0;
};

} // namespace netserver
