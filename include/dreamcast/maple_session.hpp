#pragma once

#include <cstdint>
#include <vector>

#include "dreamcast/controller_device.hpp"
#include "dreamcast/maple_pio_buffer.hpp"
#include "dreamcast/maple_wire.hpp"

namespace dreamcast {

enum class MapleSessionStatus {
    NoResponse,
    Respond,
    Incomplete,
    BadChecksum,
};

struct MapleSessionResult {
    MapleSessionStatus status = MapleSessionStatus::NoResponse;
    MaplePacket request;
    MaplePacket response;
    MaplePioTxBuffer tx_buffer;
};

class ControllerMapleSession {
public:
    explicit ControllerMapleSession(ControllerDevice controller = ControllerDevice{});

    ControllerDevice& controller();
    const ControllerDevice& controller() const;

    MapleSessionResult handle_host_bits(const std::uint8_t* bits, std::size_t bit_count) const;
    MapleSessionResult handle_host_bits(const std::vector<std::uint8_t>& bits) const;

private:
    ControllerDevice controller_;
};

}  // namespace dreamcast

