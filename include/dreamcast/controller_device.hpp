#pragma once

#include <cstdint>

#include "dreamcast/dreamcast_controller.hpp"

namespace dreamcast {

class ControllerDevice {
public:
    explicit ControllerDevice(std::uint8_t port = 0,
                              ControllerIdentity identity = ControllerIdentity{});

    std::uint8_t port() const;
    std::uint8_t host_address() const;
    std::uint8_t device_address() const;

    const ControllerState& state() const;
    ControllerState& mutable_state();
    void set_state(const ControllerState& state);

    const ControllerIdentity& identity() const;
    void set_identity(const ControllerIdentity& identity);

    MaplePacket handle_request(const MaplePacket& request) const;

private:
    std::uint8_t port_;
    ControllerState state_;
    ControllerIdentity identity_;
};

}  // namespace dreamcast

