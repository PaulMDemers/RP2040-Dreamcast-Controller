#include "dreamcast/maple_wire.hpp"

#include <algorithm>

namespace dreamcast {

namespace {

void append_byte_bits(std::vector<std::uint8_t>& bits, std::uint8_t byte) {
    for (int bit = 7; bit >= 0; --bit) {
        bits.push_back(static_cast<std::uint8_t>((byte >> bit) & 0x01u));
    }
}

void append_word_bits(std::vector<std::uint8_t>& bits, std::uint32_t word) {
    append_byte_bits(bits, static_cast<std::uint8_t>((word >> 24u) & 0xFFu));
    append_byte_bits(bits, static_cast<std::uint8_t>((word >> 16u) & 0xFFu));
    append_byte_bits(bits, static_cast<std::uint8_t>((word >> 8u) & 0xFFu));
    append_byte_bits(bits, static_cast<std::uint8_t>(word & 0xFFu));
}

std::uint32_t read_word_bits(const std::uint8_t* bits) {
    std::uint32_t word = 0;
    for (std::size_t i = 0; i < 32; ++i) {
        word = static_cast<std::uint32_t>((word << 1u) | (bits[i] & 0x01u));
    }
    return word;
}

std::uint8_t read_byte_bits(const std::uint8_t* bits) {
    std::uint8_t byte = 0;
    for (std::size_t i = 0; i < 8; ++i) {
        byte = static_cast<std::uint8_t>((byte << 1u) | (bits[i] & 0x01u));
    }
    return byte;
}

}  // namespace

MapleBitstream encode_maple_bitstream(const MaplePacket& packet) {
    MapleBitstream stream;
    const auto wire_words = packet.to_words(PacketByteOrder::Wire);
    stream.checksum = maple_checksum(packet);
    stream.bits.reserve((wire_words.size() * 32u) + 8u);

    for (std::uint32_t word : wire_words) {
        append_word_bits(stream.bits, word);
    }
    append_byte_bits(stream.bits, stream.checksum);

    return stream;
}

MapleWireDecodeResult decode_maple_bitstream(const std::uint8_t* bits, std::size_t bit_count) {
    MapleWireDecodeResult result;
    if (bits == nullptr || bit_count < 40) {
        return result;
    }

    const std::uint32_t frame_word = read_word_bits(bits);
    const MapleFrame frame = MapleFrame::from_word(frame_word, PacketByteOrder::Wire);
    const std::size_t packet_word_count = static_cast<std::size_t>(frame.payload_words) + 1u;
    const std::size_t required_bits = (packet_word_count * 32u) + 8u;
    result.consumed_bits = std::min(bit_count, required_bits);

    if (bit_count < required_bits) {
        return result;
    }

    std::vector<std::uint32_t> wire_words;
    wire_words.reserve(packet_word_count);
    for (std::size_t word_index = 0; word_index < packet_word_count; ++word_index) {
        wire_words.push_back(read_word_bits(bits + (word_index * 32u)));
    }

    result.packet = MaplePacket::from_words(wire_words.data(), wire_words.size(), PacketByteOrder::Wire);
    result.actual_checksum = read_byte_bits(bits + (packet_word_count * 32u));
    result.expected_checksum = maple_checksum(result.packet);
    result.enough_bits = true;
    result.checksum_ok = result.actual_checksum == result.expected_checksum;
    result.consumed_bits = required_bits;
    return result;
}

MapleWireDecodeResult decode_maple_bitstream(const std::vector<std::uint8_t>& bits) {
    return decode_maple_bitstream(bits.data(), bits.size());
}

}  // namespace dreamcast

