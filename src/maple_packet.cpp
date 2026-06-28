#include "dreamcast/maple_packet.hpp"

#include <stdexcept>

namespace dreamcast {

namespace {

constexpr std::uint32_t command_shift(PacketByteOrder order) {
    return order == PacketByteOrder::Host ? 24u : 0u;
}

constexpr std::uint32_t recipient_shift(PacketByteOrder order) {
    return order == PacketByteOrder::Host ? 16u : 8u;
}

constexpr std::uint32_t sender_shift(PacketByteOrder order) {
    return order == PacketByteOrder::Host ? 8u : 16u;
}

constexpr std::uint32_t length_shift(PacketByteOrder order) {
    return order == PacketByteOrder::Host ? 0u : 24u;
}

std::uint8_t byte_at(std::uint32_t word, std::uint32_t byte_index) {
    return static_cast<std::uint8_t>((word >> (byte_index * 8u)) & 0xFFu);
}

}  // namespace

MapleFrame MapleFrame::from_word(std::uint32_t word, PacketByteOrder byte_order) {
    MapleFrame frame;
    frame.command = static_cast<MapleCommand>((word >> command_shift(byte_order)) & 0xFFu);
    frame.recipient = static_cast<std::uint8_t>((word >> recipient_shift(byte_order)) & 0xFFu);
    frame.sender = static_cast<std::uint8_t>((word >> sender_shift(byte_order)) & 0xFFu);
    frame.payload_words = static_cast<std::uint8_t>((word >> length_shift(byte_order)) & 0xFFu);
    return frame;
}

std::uint32_t MapleFrame::to_word(PacketByteOrder byte_order) const {
    return (static_cast<std::uint32_t>(command) << command_shift(byte_order)) |
           (static_cast<std::uint32_t>(recipient) << recipient_shift(byte_order)) |
           (static_cast<std::uint32_t>(sender) << sender_shift(byte_order)) |
           (static_cast<std::uint32_t>(payload_words) << length_shift(byte_order));
}

MaplePacket::MaplePacket(MapleFrame packet_frame) : frame(packet_frame) {
    frame.payload_words = 0;
}

MaplePacket::MaplePacket(MapleFrame packet_frame, std::vector<std::uint32_t> packet_payload)
    : frame(packet_frame), payload(std::move(packet_payload)) {
    if (payload.size() > 255) {
        throw std::length_error("Maple payload cannot exceed 255 words");
    }
    frame.payload_words = static_cast<std::uint8_t>(payload.size());
}

MaplePacket MaplePacket::from_words(const std::uint32_t* words,
                                    std::size_t word_count,
                                    PacketByteOrder byte_order) {
    if (word_count == 0) {
        return MaplePacket{};
    }

    MapleFrame packet_frame = MapleFrame::from_word(words[0], byte_order);
    std::vector<std::uint32_t> packet_payload;
    packet_payload.reserve(word_count - 1);
    for (std::size_t i = 1; i < word_count; ++i) {
        packet_payload.push_back(byte_order == PacketByteOrder::Host ? words[i] : byte_swap_word(words[i]));
    }

    return MaplePacket(packet_frame, std::move(packet_payload));
}

std::vector<std::uint32_t> MaplePacket::to_words(PacketByteOrder byte_order) const {
    MapleFrame output_frame = frame;
    output_frame.payload_words = static_cast<std::uint8_t>(payload.size());

    std::vector<std::uint32_t> words;
    words.reserve(payload.size() + 1);
    words.push_back(output_frame.to_word(byte_order));
    for (std::uint32_t word : payload) {
        words.push_back(byte_order == PacketByteOrder::Host ? word : byte_swap_word(word));
    }
    return words;
}

bool MaplePacket::is_valid() const {
    return frame.command != MapleCommand::Invalid && payload.size() <= 255 &&
           frame.payload_words == payload.size();
}

std::uint32_t byte_swap_word(std::uint32_t word) {
    return ((word & 0x000000FFu) << 24u) |
           ((word & 0x0000FF00u) << 8u) |
           ((word & 0x00FF0000u) >> 8u) |
           ((word & 0xFF000000u) >> 24u);
}

std::uint8_t maple_checksum(const std::uint32_t* words,
                            std::size_t word_count,
                            PacketByteOrder byte_order) {
    std::uint8_t checksum = 0;
    for (std::size_t i = 0; i < word_count; ++i) {
        const std::uint32_t word = byte_order == PacketByteOrder::Host ? words[i] : byte_swap_word(words[i]);
        checksum ^= byte_at(word, 3);
        checksum ^= byte_at(word, 2);
        checksum ^= byte_at(word, 1);
        checksum ^= byte_at(word, 0);
    }
    return checksum;
}

std::uint8_t maple_checksum(const MaplePacket& packet, PacketByteOrder byte_order) {
    const auto words = packet.to_words(byte_order);
    return maple_checksum(words.data(), words.size(), byte_order);
}

}  // namespace dreamcast

