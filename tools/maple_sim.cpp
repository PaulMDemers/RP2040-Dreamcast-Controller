#include <iomanip>
#include <iostream>

#include "dreamcast/controller_device.hpp"

namespace {

void print_packet(const char* label, const dreamcast::MaplePacket& packet) {
    const auto words = packet.to_words();
    std::cout << label << '\n';
    for (std::uint32_t word : words) {
        std::cout << "  0x" << std::hex << std::uppercase << std::setw(8)
                  << std::setfill('0') << word << std::dec << '\n';
    }
    std::cout << "  checksum: 0x" << std::hex << std::uppercase << std::setw(2)
              << std::setfill('0') << static_cast<unsigned>(dreamcast::maple_checksum(packet))
              << std::dec << "\n\n";
}

}  // namespace

int main() {
    dreamcast::ControllerDevice controller(0);
    controller.mutable_state().set_pressed(dreamcast::ButtonA, true);
    controller.mutable_state().set_pressed(dreamcast::ButtonStart, true);
    controller.mutable_state().left_x = 180;
    controller.mutable_state().left_y = 80;
    controller.mutable_state().right_trigger = 200;

    const dreamcast::MaplePacket info_request(
        dreamcast::MapleFrame{dreamcast::MapleCommand::DeviceInfoRequest,
                              controller.device_address(), controller.host_address(), 0},
        {});
    const dreamcast::MaplePacket condition_request(
        dreamcast::MapleFrame{dreamcast::MapleCommand::GetCondition,
                              controller.device_address(), controller.host_address(), 0},
        {dreamcast::kFunctionController});

    print_packet("Device info request", info_request);
    print_packet("Device info response", controller.handle_request(info_request));
    print_packet("Get condition request", condition_request);
    print_packet("Get condition response", controller.handle_request(condition_request));

    return 0;
}
