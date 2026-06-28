#include "dreamcast/maple_pio_buffer.hpp"

namespace dreamcast {

MaplePioTxBuffer build_maple_pio_tx_buffer(const MaplePacket& packet) {
    MaplePioTxBuffer buffer;
    buffer.bit_count = static_cast<std::uint32_t>(((packet.payload.size() + 1u) * 32u) + 8u);
    buffer.words = packet.to_words(PacketByteOrder::Wire);
    buffer.words.push_back(static_cast<std::uint32_t>(maple_checksum(packet)) << 24u);
    return buffer;
}

}  // namespace dreamcast

