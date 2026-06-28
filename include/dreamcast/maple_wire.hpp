#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "dreamcast/maple_packet.hpp"

namespace dreamcast {

struct MapleBitstream {
    std::vector<std::uint8_t> bits;
    std::uint8_t checksum = 0;
};

struct MapleWireDecodeResult {
    MaplePacket packet;
    bool enough_bits = false;
    bool checksum_ok = false;
    std::uint8_t expected_checksum = 0;
    std::uint8_t actual_checksum = 0;
    std::size_t consumed_bits = 0;
};

MapleBitstream encode_maple_bitstream(const MaplePacket& packet);
MapleWireDecodeResult decode_maple_bitstream(const std::uint8_t* bits, std::size_t bit_count);
MapleWireDecodeResult decode_maple_bitstream(const std::vector<std::uint8_t>& bits);

}  // namespace dreamcast

