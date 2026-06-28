#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "dreamcast/maple_constants.hpp"

namespace dreamcast {

enum class PacketByteOrder {
    Host,
    Wire,
};

struct MapleFrame {
    MapleCommand command = MapleCommand::Invalid;
    std::uint8_t recipient = 0;
    std::uint8_t sender = 0;
    std::uint8_t payload_words = 0;

    static MapleFrame from_word(std::uint32_t word, PacketByteOrder byte_order = PacketByteOrder::Host);
    std::uint32_t to_word(PacketByteOrder byte_order = PacketByteOrder::Host) const;
};

struct MaplePacket {
    MapleFrame frame;
    std::vector<std::uint32_t> payload;

    MaplePacket() = default;
    explicit MaplePacket(MapleFrame packet_frame);
    MaplePacket(MapleFrame packet_frame, std::vector<std::uint32_t> packet_payload);

    static MaplePacket from_words(const std::uint32_t* words,
                                  std::size_t word_count,
                                  PacketByteOrder byte_order = PacketByteOrder::Host);

    std::vector<std::uint32_t> to_words(PacketByteOrder byte_order = PacketByteOrder::Host) const;
    bool is_valid() const;
};

std::uint32_t byte_swap_word(std::uint32_t word);
std::uint8_t maple_checksum(const std::uint32_t* words,
                            std::size_t word_count,
                            PacketByteOrder byte_order = PacketByteOrder::Host);
std::uint8_t maple_checksum(const MaplePacket& packet,
                            PacketByteOrder byte_order = PacketByteOrder::Host);

}  // namespace dreamcast

