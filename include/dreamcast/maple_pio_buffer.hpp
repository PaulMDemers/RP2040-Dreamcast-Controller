#pragma once

#include <cstdint>
#include <vector>

#include "dreamcast/maple_packet.hpp"

namespace dreamcast {

struct MaplePioTxBuffer {
    std::uint32_t bit_count = 0;
    std::vector<std::uint32_t> words;
};

MaplePioTxBuffer build_maple_pio_tx_buffer(const MaplePacket& packet);

}  // namespace dreamcast

