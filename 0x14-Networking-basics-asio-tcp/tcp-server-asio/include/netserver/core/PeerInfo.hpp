#pragma once

#include <cstdint>
#include <string>

namespace netserver {

enum class TransportType {
    Tcp,
    Udp
};

// information about a peer (client)
struct PeerInfo {
    TransportType transport{TransportType::Tcp};
    std::string address;
    std::uint16_t port{0};
};

} // namespace netserver
