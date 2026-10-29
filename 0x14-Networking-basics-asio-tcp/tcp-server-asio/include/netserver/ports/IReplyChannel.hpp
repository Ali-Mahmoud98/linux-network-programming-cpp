#pragma once

#include "netserver/core/Message.hpp"

namespace netserver {

class IReplyChannel {
public:
    virtual ~IReplyChannel() = default;
    virtual void send(Message message) = 0;
};

} // namespace netserver
