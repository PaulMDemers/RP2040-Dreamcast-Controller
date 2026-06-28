#include "dreamcast/controller_device.hpp"

#include <utility>

namespace dreamcast {

ControllerDevice::ControllerDevice(std::uint8_t port, ControllerIdentity identity)
    : port_(static_cast<std::uint8_t>(port & 0x03u)), identity_(std::move(identity)) {}

std::uint8_t ControllerDevice::port() const {
    return port_;
}

std::uint8_t ControllerDevice::host_address() const {
    return make_host_address(port_);
}

std::uint8_t ControllerDevice::device_address() const {
    return make_main_peripheral_address(port_);
}

const ControllerState& ControllerDevice::state() const {
    return state_;
}

ControllerState& ControllerDevice::mutable_state() {
    return state_;
}

void ControllerDevice::set_state(const ControllerState& state) {
    state_ = state;
}

const ControllerIdentity& ControllerDevice::identity() const {
    return identity_;
}

void ControllerDevice::set_identity(const ControllerIdentity& identity) {
    identity_ = identity;
}

MaplePacket ControllerDevice::handle_request(const MaplePacket& request) const {
    return dispatch_standard_controller_request(request, device_address(), state_, identity_);
}

}  // namespace dreamcast
