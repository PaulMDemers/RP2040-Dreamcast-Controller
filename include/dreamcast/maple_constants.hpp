#pragma once

#include <cstdint>

namespace dreamcast {

constexpr std::uint32_t kFunctionController = 0x00000001u;
constexpr std::uint32_t kFunctionStorage = 0x00000002u;
constexpr std::uint32_t kFunctionLcd = 0x00000004u;
constexpr std::uint32_t kFunctionTimer = 0x00000008u;
constexpr std::uint32_t kFunctionAudioInput = 0x00000010u;
constexpr std::uint32_t kFunctionArGun = 0x00000020u;
constexpr std::uint32_t kFunctionKeyboard = 0x00000040u;
constexpr std::uint32_t kFunctionGun = 0x00000080u;
constexpr std::uint32_t kFunctionVibration = 0x00000100u;
constexpr std::uint32_t kFunctionMouse = 0x00000200u;
constexpr std::uint32_t kFunctionExtStorage = 0x00000400u;
constexpr std::uint32_t kFunctionCamera = 0x00000800u;

enum class MapleCommand : std::uint8_t {
    DeviceInfoRequest = 0x01,
    ExtendedDeviceInfoRequest = 0x02,
    Reset = 0x03,
    Shutdown = 0x04,
    ResponseDeviceInfo = 0x05,
    ResponseExtendedDeviceInfo = 0x06,
    ResponseAck = 0x07,
    ResponseDataTransfer = 0x08,
    GetCondition = 0x09,
    GetMemoryInformation = 0x0A,
    BlockRead = 0x0B,
    BlockWrite = 0x0C,
    GetLastError = 0x0D,
    SetCondition = 0x0E,
    ResponseFileError = 0xFB,
    ResponseRequestResend = 0xFC,
    ResponseUnknownCommand = 0xFD,
    ResponseFunctionNotSupported = 0xFE,
    Invalid = 0xFF,
};

constexpr std::uint8_t kAddressMainPeripheral = 0x20u;
constexpr std::uint8_t kAddressSubPeripheralMask = 0x1Fu;
constexpr std::uint8_t kAddressUnitMask = 0x3Fu;
constexpr std::uint8_t kAddressPortShift = 6u;
constexpr std::uint8_t kAddressPortMask = 0xC0u;

constexpr std::uint8_t make_host_address(std::uint8_t port) {
    return static_cast<std::uint8_t>((port & 0x03u) << kAddressPortShift);
}

constexpr std::uint8_t make_main_peripheral_address(std::uint8_t port, std::uint8_t sub_units = 0) {
    return static_cast<std::uint8_t>(
        make_host_address(port) | kAddressMainPeripheral | (sub_units & kAddressSubPeripheralMask));
}

constexpr std::uint8_t address_port(std::uint8_t address) {
    return static_cast<std::uint8_t>((address & kAddressPortMask) >> kAddressPortShift);
}

constexpr bool address_targets_main_peripheral(std::uint8_t address) {
    return (address & kAddressMainPeripheral) != 0;
}

}  // namespace dreamcast

