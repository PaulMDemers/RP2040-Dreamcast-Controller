#include "dreamcast/controller_device.hpp"
#include "dreamcast/controller_input_parser.hpp"

#include <cstddef>

#ifdef PICO_ON_DEVICE
#include "dreamcast/pico/maple_phy.hpp"
#include "pico/stdlib.h"
#endif

int main() {
#ifdef PICO_ON_DEVICE
    stdio_init_all();
    sleep_ms(250);
#endif

    dreamcast::ControllerDevice controller;
    controller.mutable_state().left_x = 128;
    controller.mutable_state().left_y = 128;

#ifdef PICO_ON_DEVICE
    dreamcast::pico::MaplePhyConfig maple_config;
    maple_config.pin_a = 10;
    maple_config.sys_clock_khz = 133000;
    maple_config.ns_per_bit = 480;
    maple_config.idle_check_us = 10;
    dreamcast::pico::PicoMaplePhy maple_phy(maple_config);
    maple_phy.init();

    char command_buffer[96] = {};
    std::size_t command_len = 0;
#endif

    while (true) {
#ifdef PICO_ON_DEVICE
        const int ch = getchar_timeout_us(0);
        if (ch == '\r' || ch == '\n') {
            if (command_len > 0) {
                command_buffer[command_len] = '\0';
                dreamcast::apply_controller_input_command(controller.mutable_state(), command_buffer);
                command_len = 0;
            }
        } else if (ch >= 0 && command_len + 1 < sizeof(command_buffer)) {
            command_buffer[command_len++] = static_cast<char>(ch);
        }

        tight_loop_contents();
#else
        break;
#endif
    }

    return 0;
}
