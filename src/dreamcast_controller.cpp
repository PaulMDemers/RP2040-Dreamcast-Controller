#include "dreamcast/dreamcast_controller.hpp"

#include <algorithm>
#include <array>

namespace dreamcast {

namespace {

std::uint16_t active_low_buttons(const ControllerState& state) {
    return static_cast<std::uint16_t>(~state.pressed_buttons);
}

void copy_fixed_text(std::uint8_t* dest, std::size_t len, const std::string& value) {
    std::fill(dest, dest + len, static_cast<std::uint8_t>(' '));
    const std::size_t copy_len = std::min(len, value.size());
    for (std::size_t i = 0; i < copy_len; ++i) {
        dest[i] = static_cast<std::uint8_t>(value[i]);
    }
}

std::array<std::uint32_t, 28> bytes_to_words(const std::array<std::uint8_t, 112>& bytes) {
    std::array<std::uint32_t, 28> words{};
    for (std::size_t word = 0; word < words.size(); ++word) {
        const std::size_t offset = word * 4;
        words[word] = (static_cast<std::uint32_t>(bytes[offset]) << 24u) |
                      (static_cast<std::uint32_t>(bytes[offset + 1]) << 16u) |
                      (static_cast<std::uint32_t>(bytes[offset + 2]) << 8u) |
                      static_cast<std::uint32_t>(bytes[offset + 3]);
    }
    return words;
}

MaplePacket simple_response(MapleCommand command, std::uint8_t recipient, std::uint8_t sender) {
    return MaplePacket(MapleFrame{command, recipient, sender, 0}, {});
}

}  // namespace

void ControllerState::set_pressed(ControllerButton button, bool pressed) {
    if (pressed) {
        pressed_buttons = static_cast<std::uint16_t>(pressed_buttons | button);
    } else {
        pressed_buttons = static_cast<std::uint16_t>(pressed_buttons & ~button);
    }
}

bool ControllerState::is_pressed(ControllerButton button) const {
    return (pressed_buttons & button) != 0;
}

std::uint32_t pack_controller_condition_word_1(const ControllerState& state) {
    const std::uint16_t buttons = active_low_buttons(state);
    const std::uint8_t low_button_byte = static_cast<std::uint8_t>(buttons & 0xFFu);
    const std::uint8_t high_button_byte = static_cast<std::uint8_t>((buttons >> 8u) & 0xFFu);
    return (static_cast<std::uint32_t>(low_button_byte) << 24u) |
           (static_cast<std::uint32_t>(high_button_byte) << 16u) |
           (static_cast<std::uint32_t>(state.right_trigger) << 8u) |
           static_cast<std::uint32_t>(state.left_trigger);
}

std::uint32_t pack_controller_condition_word_2(const ControllerState& state) {
    return (static_cast<std::uint32_t>(state.left_x) << 24u) |
           (static_cast<std::uint32_t>(state.left_y) << 16u) |
           (static_cast<std::uint32_t>(state.right_x) << 8u) |
           static_cast<std::uint32_t>(state.right_y);
}

std::array<std::uint32_t, 28> build_controller_device_info_payload(
    const ControllerIdentity& identity,
    std::uint8_t attached_sub_units) {
    std::array<std::uint8_t, 112> bytes{};

    const std::uint32_t functions = kFunctionController;
    bytes[0] = static_cast<std::uint8_t>((functions >> 24u) & 0xFFu);
    bytes[1] = static_cast<std::uint8_t>((functions >> 16u) & 0xFFu);
    bytes[2] = static_cast<std::uint8_t>((functions >> 8u) & 0xFFu);
    bytes[3] = static_cast<std::uint8_t>(functions & 0xFFu);

    const std::uint32_t function_data = kControllerStandardFunctionData;
    bytes[4] = static_cast<std::uint8_t>((function_data >> 24u) & 0xFFu);
    bytes[5] = static_cast<std::uint8_t>((function_data >> 16u) & 0xFFu);
    bytes[6] = static_cast<std::uint8_t>((function_data >> 8u) & 0xFFu);
    bytes[7] = static_cast<std::uint8_t>(function_data & 0xFFu);

    bytes[16] = identity.area_code;
    (void)attached_sub_units;
    bytes[17] = identity.connector_direction;
    copy_fixed_text(&bytes[18], 30, identity.product_name);
    copy_fixed_text(&bytes[48], 60, identity.product_license);

    bytes[108] = static_cast<std::uint8_t>(identity.standby_power_ma & 0xFFu);
    bytes[109] = static_cast<std::uint8_t>((identity.standby_power_ma >> 8u) & 0xFFu);
    bytes[110] = static_cast<std::uint8_t>(identity.max_power_ma & 0xFFu);
    bytes[111] = static_cast<std::uint8_t>((identity.max_power_ma >> 8u) & 0xFFu);

    return bytes_to_words(bytes);
}

MaplePacket build_controller_device_info_response(std::uint8_t recipient,
                                                   std::uint8_t sender,
                                                   const ControllerIdentity& identity,
                                                   std::uint8_t attached_sub_units) {
    const auto payload = build_controller_device_info_payload(identity, attached_sub_units);
    return MaplePacket(
        MapleFrame{MapleCommand::ResponseDeviceInfo, recipient,
                   static_cast<std::uint8_t>(sender | (attached_sub_units & kAddressSubPeripheralMask)), 0},
        std::vector<std::uint32_t>(payload.begin(), payload.end()));
}

MaplePacket build_controller_condition_response(std::uint8_t recipient,
                                                std::uint8_t sender,
                                                const ControllerState& state) {
    return MaplePacket(
        MapleFrame{MapleCommand::ResponseDataTransfer, recipient, sender, 0},
        {kFunctionController, pack_controller_condition_word_1(state),
         pack_controller_condition_word_2(state)});
}

MaplePacket dispatch_standard_controller_request(const MaplePacket& request,
                                                 std::uint8_t device_address,
                                                 const ControllerState& state,
                                                 const ControllerIdentity& identity) {
    const std::uint8_t host_address = request.frame.sender;
    if (request.frame.recipient != device_address ||
        !address_targets_main_peripheral(request.frame.recipient)) {
        return MaplePacket{};
    }

    switch (request.frame.command) {
        case MapleCommand::DeviceInfoRequest:
            return build_controller_device_info_response(host_address, device_address, identity);

        case MapleCommand::ExtendedDeviceInfoRequest:
            return build_controller_device_info_response(host_address, device_address, identity);

        case MapleCommand::Reset:
        case MapleCommand::Shutdown:
            return simple_response(MapleCommand::ResponseAck, host_address, device_address);

        case MapleCommand::GetCondition:
            if (request.payload.empty() || request.payload[0] != kFunctionController) {
                return simple_response(MapleCommand::ResponseFunctionNotSupported, host_address, device_address);
            }
            return build_controller_condition_response(host_address, device_address, state);

        default:
            return simple_response(MapleCommand::ResponseUnknownCommand, host_address, device_address);
    }
}

}  // namespace dreamcast
