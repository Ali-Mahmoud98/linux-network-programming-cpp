#pragma once

#include <functional>
#include "netserver/ports/InboundMessage.hpp"

namespace netserver {

// The boundary through which a transport adapter invokes the application.
using InboundMessageHandler = std::function<void(const InboundMessage&)>;

} // namespace netserver
