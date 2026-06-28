#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "dreamcast/controller_device.hpp"
#include "dreamcast/controller_input_parser.hpp"
#include "dreamcast/dreamcast_controller.hpp"
#include "dreamcast/maple_pio_buffer.hpp"
#include "dreamcast/maple_session.hpp"
#include "dreamcast/maple_wire.hpp"

namespace {

int g_failures = 0;

void expect_true(bool condition, const std::string& label) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << label << '\n';
    }
}

template <typename T, typename U>
void expect_eq(const T& actual, const U& expected, const std::string& label) {
    if (!(actual == expected)) {
        ++g_failures;
        std::cerr << "FAIL: " << label << " expected=" << +expected
                  << " actual=" << +actual << '\n';
    }
}

}  // namespace

int main() {
    using namespace dreamcast;

    {
        MapleFrame frame{MapleCommand::GetCondition, 0x20, 0x00, 1};
        expect_eq(frame.to_word(), 0x09200001u, "frame host word");

        const MapleFrame parsed = MapleFrame::from_word(0x09200001u);
        expect_true(parsed.command == MapleCommand::GetCondition, "frame parsed command");
        expect_eq(parsed.recipient, 0x20u, "frame parsed recipient");
        expect_eq(parsed.sender, 0x00u, "frame parsed sender");
        expect_eq(parsed.payload_words, 1u, "frame parsed length");
    }

    {
        expect_eq(byte_swap_word(0x12345678u), 0x78563412u, "byte swap");

        MaplePacket packet(MapleFrame{MapleCommand::GetCondition, 0x20, 0x00, 0},
                           {kFunctionController});
        const auto host_words = packet.to_words();
        expect_eq(host_words.size(), std::size_t{2}, "packet word count");
        expect_eq(host_words[0], 0x09200001u, "packet corrected length");
        expect_eq(host_words[1], 0x00000001u, "packet payload");

        const auto wire_words = packet.to_words(PacketByteOrder::Wire);
        expect_eq(wire_words[0], 0x01002009u, "wire frame word");
        expect_eq(wire_words[1], 0x01000000u, "wire payload word");

        const MaplePacket reparsed = MaplePacket::from_words(wire_words.data(), wire_words.size(),
                                                             PacketByteOrder::Wire);
        expect_true(reparsed.frame.command == MapleCommand::GetCondition, "wire reparse command");
        expect_eq(reparsed.payload[0], kFunctionController, "wire reparse payload");
    }

    {
        const std::uint32_t words[] = {0x09200001u, 0x00000001u};
        expect_eq(maple_checksum(words, 2), static_cast<std::uint8_t>(0x29), "checksum");
    }

    {
        MaplePacket packet(MapleFrame{MapleCommand::GetCondition, 0x20, 0x00, 0},
                           {kFunctionController});
        const MapleBitstream stream = encode_maple_bitstream(packet);
        expect_eq(stream.bits.size(), std::size_t{72}, "bitstream bit count");
        expect_eq(stream.checksum, static_cast<std::uint8_t>(0x29), "bitstream checksum");

        const std::uint8_t first_frame_byte = static_cast<std::uint8_t>(
            (stream.bits[0] << 7u) | (stream.bits[1] << 6u) | (stream.bits[2] << 5u) |
            (stream.bits[3] << 4u) | (stream.bits[4] << 3u) | (stream.bits[5] << 2u) |
            (stream.bits[6] << 1u) | stream.bits[7]);
        expect_eq(first_frame_byte, 0x01u, "bitstream starts with wire length byte");

        const MapleWireDecodeResult decoded = decode_maple_bitstream(stream.bits);
        expect_true(decoded.enough_bits, "decode has enough bits");
        expect_true(decoded.checksum_ok, "decode checksum ok");
        expect_eq(decoded.consumed_bits, stream.bits.size(), "decode consumed bits");
        expect_true(decoded.packet.frame.command == MapleCommand::GetCondition,
                    "decode command");
        expect_eq(decoded.packet.payload[0], kFunctionController, "decode payload");

        std::vector<std::uint8_t> truncated(stream.bits.begin(), stream.bits.end() - 1);
        const MapleWireDecodeResult short_decode = decode_maple_bitstream(truncated);
        expect_true(!short_decode.enough_bits, "decode catches truncated stream");

        std::vector<std::uint8_t> corrupted = stream.bits;
        corrupted.back() ^= 0x01u;
        const MapleWireDecodeResult corrupt_decode = decode_maple_bitstream(corrupted);
        expect_true(corrupt_decode.enough_bits, "decode corrupted has enough bits");
        expect_true(!corrupt_decode.checksum_ok, "decode catches bad checksum");
    }

    {
        MaplePacket packet(MapleFrame{MapleCommand::GetCondition, 0x20, 0x00, 0},
                           {kFunctionController});
        const MaplePioTxBuffer buffer = build_maple_pio_tx_buffer(packet);
        expect_eq(buffer.bit_count, 72u, "pio tx bit count");
        expect_eq(buffer.words.size(), std::size_t{3}, "pio tx words include crc word");
        expect_eq(buffer.words[0], 0x01002009u, "pio tx frame is wire order");
        expect_eq(buffer.words[1], 0x01000000u, "pio tx payload is wire order");
        expect_eq(buffer.words[2], 0x29000000u, "pio tx crc high byte");
    }

    {
        ControllerState state;
        expect_eq(pack_controller_condition_word_1(state), 0xFFFF0000u, "neutral buttons");
        expect_eq(pack_controller_condition_word_2(state), 0x80808080u, "neutral axes");

        state.set_pressed(ButtonA, true);
        state.set_pressed(ButtonStart, true);
        state.left_trigger = 12;
        state.right_trigger = 240;
        state.left_x = 1;
        state.left_y = 2;
        state.right_x = 3;
        state.right_y = 4;

        expect_true(state.is_pressed(ButtonA), "A pressed");
        expect_eq(pack_controller_condition_word_1(state), 0xF3FFF00Cu,
                  "active-low A+Start with triggers");
        expect_eq(pack_controller_condition_word_2(state), 0x01020304u, "axis packing");
    }

    {
        const auto info = build_controller_device_info_payload();
        expect_eq(info[0], kFunctionController, "device info function");
        expect_eq(info[1], kControllerStandardFunctionData, "device info controller data");
        expect_eq((info[4] >> 24u) & 0xFFu, 0xFFu, "device info area code");
    }

    {
        ControllerState state;
        state.set_pressed(ButtonB, true);
        const std::uint8_t host = make_host_address(0);
        const std::uint8_t device = make_main_peripheral_address(0);

        MaplePacket request(MapleFrame{MapleCommand::GetCondition, device, host, 0},
                            {kFunctionController});
        const MaplePacket response = dispatch_standard_controller_request(request, device, state);
        expect_true(response.frame.command == MapleCommand::ResponseDataTransfer,
                    "dispatch condition command");
        expect_eq(response.frame.recipient, host, "dispatch response recipient");
        expect_eq(response.frame.sender, device, "dispatch response sender");
        expect_eq(response.payload.size(), std::size_t{3}, "dispatch condition payload size");
        expect_eq(response.payload[1], 0xFDFF0000u, "dispatch active-low B");

        MaplePacket bad_function(MapleFrame{MapleCommand::GetCondition, device, host, 0},
                                 {kFunctionMouse});
        const MaplePacket bad_response = dispatch_standard_controller_request(bad_function, device, state);
        expect_true(bad_response.frame.command == MapleCommand::ResponseFunctionNotSupported,
                    "dispatch unsupported function");
    }

    {
        ControllerDevice device(1);
        expect_eq(device.port(), 1u, "device port");
        expect_eq(device.host_address(), 0x40u, "device host address");
        expect_eq(device.device_address(), 0x60u, "device peripheral address");

        device.mutable_state().set_pressed(ButtonX, true);
        MaplePacket request(MapleFrame{MapleCommand::GetCondition, device.device_address(),
                                       device.host_address(), 0},
                            {kFunctionController});
        const MaplePacket response = device.handle_request(request);
        expect_true(response.frame.command == MapleCommand::ResponseDataTransfer,
                    "device object condition response");
        expect_eq(response.payload[1], 0xFFFB0000u, "device object active-low X");

        MaplePacket wrong_port(MapleFrame{MapleCommand::GetCondition, 0x20, 0x00, 0},
                               {kFunctionController});
        const MaplePacket ignored = device.handle_request(wrong_port);
        expect_true(!ignored.is_valid(), "device ignores other ports");
    }

    {
        ControllerMapleSession session(ControllerDevice{0});
        session.controller().mutable_state().set_pressed(ButtonA, true);

        MaplePacket request(MapleFrame{MapleCommand::GetCondition,
                                       session.controller().device_address(),
                                       session.controller().host_address(), 0},
                            {kFunctionController});
        const MapleBitstream host_bits = encode_maple_bitstream(request);
        const MapleSessionResult result = session.handle_host_bits(host_bits.bits);
        expect_true(result.status == MapleSessionStatus::Respond, "session responds");
        expect_true(result.response.frame.command == MapleCommand::ResponseDataTransfer,
                    "session condition response");
        expect_eq(result.tx_buffer.bit_count, 136u, "session response tx bit count");
        expect_eq(result.tx_buffer.words.back(), 0x2E000000u, "session response crc word");

        std::vector<std::uint8_t> corrupted = host_bits.bits;
        corrupted.back() ^= 0x01u;
        const MapleSessionResult bad_crc = session.handle_host_bits(corrupted);
        expect_true(bad_crc.status == MapleSessionStatus::BadChecksum,
                    "session reports bad checksum");
        expect_true(bad_crc.response.frame.command == MapleCommand::ResponseRequestResend,
                    "session asks resend on bad checksum");

        std::vector<std::uint8_t> truncated(host_bits.bits.begin(), host_bits.bits.end() - 4);
        const MapleSessionResult short_packet = session.handle_host_bits(truncated);
        expect_true(short_packet.status == MapleSessionStatus::Incomplete,
                    "session reports incomplete");
    }

    {
        ControllerState state;
        expect_true(apply_controller_input_command(state, "press a").status ==
                        ControllerInputParseStatus::Ok,
                    "input press ok");
        expect_true(state.is_pressed(ButtonA), "input pressed A");

        expect_true(apply_controller_input_command(state, "release a").status ==
                        ControllerInputParseStatus::Ok,
                    "input release ok");
        expect_true(!state.is_pressed(ButtonA), "input released A");

        expect_true(apply_controller_input_command(state, "set lx 200").status ==
                        ControllerInputParseStatus::Ok,
                    "input set lx ok");
        expect_eq(state.left_x, 200u, "input left x");

        expect_true(apply_controller_input_command(state, "set rt 255").status ==
                        ControllerInputParseStatus::Ok,
                    "input set rt ok");
        expect_eq(state.right_trigger, 255u, "input right trigger");

        expect_true(apply_controller_input_command(state, "neutral").status ==
                        ControllerInputParseStatus::Ok,
                    "input neutral ok");
        expect_eq(state.left_x, 128u, "input neutral left x");
        expect_eq(state.right_trigger, 0u, "input neutral right trigger");

        expect_true(apply_controller_input_command(state, "set lx 300").status ==
                        ControllerInputParseStatus::InvalidValue,
                    "input invalid value");
        expect_true(apply_controller_input_command(state, "press nope").status ==
                        ControllerInputParseStatus::UnknownControl,
                    "input unknown control");
    }

    if (g_failures != 0) {
        std::cerr << g_failures << " test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All Dreamcast core tests passed\n";
    return EXIT_SUCCESS;
}
