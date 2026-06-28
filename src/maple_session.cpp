#include "dreamcast/maple_session.hpp"

namespace dreamcast {

namespace {

MaplePacket request_resend_response(const MaplePacket& request, std::uint8_t device_address) {
    if (request.frame.recipient != device_address) {
        return MaplePacket{};
    }

    return MaplePacket(
        MapleFrame{MapleCommand::ResponseRequestResend, request.frame.sender, device_address, 0},
        {});
}

}  // namespace

ControllerMapleSession::ControllerMapleSession(ControllerDevice controller)
    : controller_(std::move(controller)) {}

ControllerDevice& ControllerMapleSession::controller() {
    return controller_;
}

const ControllerDevice& ControllerMapleSession::controller() const {
    return controller_;
}

MapleSessionResult ControllerMapleSession::handle_host_bits(const std::uint8_t* bits,
                                                            std::size_t bit_count) const {
    MapleSessionResult result;
    MapleWireDecodeResult decoded = decode_maple_bitstream(bits, bit_count);
    result.request = decoded.packet;

    if (!decoded.enough_bits) {
        result.status = MapleSessionStatus::Incomplete;
        return result;
    }

    if (!decoded.checksum_ok) {
        result.response = request_resend_response(decoded.packet, controller_.device_address());
        if (result.response.is_valid()) {
            result.tx_buffer = build_maple_pio_tx_buffer(result.response);
        }
        result.status = MapleSessionStatus::BadChecksum;
        return result;
    }

    result.response = controller_.handle_request(decoded.packet);
    if (!result.response.is_valid()) {
        result.status = MapleSessionStatus::NoResponse;
        return result;
    }

    result.tx_buffer = build_maple_pio_tx_buffer(result.response);
    result.status = MapleSessionStatus::Respond;
    return result;
}

MapleSessionResult ControllerMapleSession::handle_host_bits(
    const std::vector<std::uint8_t>& bits) const {
    return handle_host_bits(bits.data(), bits.size());
}

}  // namespace dreamcast

