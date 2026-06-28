#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "dreamcast/maple_packet.hpp"

namespace dreamcast {

enum ControllerButton : std::uint16_t {
    ButtonC = 1u << 0,
    ButtonB = 1u << 1,
    ButtonA = 1u << 2,
    ButtonStart = 1u << 3,
    ButtonDpadUp = 1u << 4,
    ButtonDpadDown = 1u << 5,
    ButtonDpadLeft = 1u << 6,
    ButtonDpadRight = 1u << 7,
    ButtonZ = 1u << 8,
    ButtonY = 1u << 9,
    ButtonX = 1u << 10,
    ButtonD = 1u << 11,
    ButtonDpad2Up = 1u << 12,
    ButtonDpad2Down = 1u << 13,
    ButtonDpad2Left = 1u << 14,
    ButtonDpad2Right = 1u << 15,
};

constexpr std::uint32_t kControllerControlC = 1u << 0;
constexpr std::uint32_t kControllerControlB = 1u << 1;
constexpr std::uint32_t kControllerControlA = 1u << 2;
constexpr std::uint32_t kControllerControlStart = 1u << 3;
constexpr std::uint32_t kControllerControlDpadUp = 1u << 4;
constexpr std::uint32_t kControllerControlDpadDown = 1u << 5;
constexpr std::uint32_t kControllerControlDpadLeft = 1u << 6;
constexpr std::uint32_t kControllerControlDpadRight = 1u << 7;
constexpr std::uint32_t kControllerControlZ = 1u << 8;
constexpr std::uint32_t kControllerControlY = 1u << 9;
constexpr std::uint32_t kControllerControlX = 1u << 10;
constexpr std::uint32_t kControllerControlD = 1u << 11;
constexpr std::uint32_t kControllerControlAnalogR = 1u << 16;
constexpr std::uint32_t kControllerControlAnalogL = 1u << 17;
constexpr std::uint32_t kControllerControlAnalogX = 1u << 18;
constexpr std::uint32_t kControllerControlAnalogY = 1u << 19;
constexpr std::uint32_t kControllerStandardFunctionData =
    kControllerControlB | kControllerControlA | kControllerControlStart |
    kControllerControlDpadUp | kControllerControlDpadDown |
    kControllerControlDpadLeft | kControllerControlDpadRight |
    kControllerControlY | kControllerControlX |
    kControllerControlAnalogR | kControllerControlAnalogL |
    kControllerControlAnalogX | kControllerControlAnalogY;

struct ControllerState {
    std::uint16_t pressed_buttons = 0;
    std::uint8_t left_trigger = 0;
    std::uint8_t right_trigger = 0;
    std::uint8_t left_x = 128;
    std::uint8_t left_y = 128;
    std::uint8_t right_x = 128;
    std::uint8_t right_y = 128;

    void set_pressed(ControllerButton button, bool pressed);
    bool is_pressed(ControllerButton button) const;
};

struct ControllerIdentity {
    std::uint8_t area_code = 0xFF;
    std::uint8_t connector_direction = 0x00;
    std::string product_name = "Dreamcast Controller";
    std::string product_license = "Produced By or Under License From SEGA ENTERPRISES,LTD.";
    std::uint16_t standby_power_ma = 0x01AE;
    std::uint16_t max_power_ma = 0x01F4;
};

std::uint32_t pack_controller_condition_word_1(const ControllerState& state);
std::uint32_t pack_controller_condition_word_2(const ControllerState& state);
std::array<std::uint32_t, 28> build_controller_device_info_payload(
    const ControllerIdentity& identity = ControllerIdentity{},
    std::uint8_t attached_sub_units = 0);
MaplePacket build_controller_device_info_response(std::uint8_t recipient,
                                                   std::uint8_t sender,
                                                   const ControllerIdentity& identity = ControllerIdentity{},
                                                   std::uint8_t attached_sub_units = 0);
MaplePacket build_controller_condition_response(std::uint8_t recipient,
                                                std::uint8_t sender,
                                                const ControllerState& state);
MaplePacket dispatch_standard_controller_request(const MaplePacket& request,
                                                 std::uint8_t device_address,
                                                 const ControllerState& state,
                                                 const ControllerIdentity& identity = ControllerIdentity{});

}  // namespace dreamcast

