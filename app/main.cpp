#include "dreamcast/controller_device.hpp"

#ifdef PICO_ON_DEVICE
#include "pico/stdlib.h"
#endif

int main() {
#ifdef PICO_ON_DEVICE
    stdio_init_all();
#endif

    dreamcast::ControllerDevice controller;
    controller.mutable_state().left_x = 128;
    controller.mutable_state().left_y = 128;

    while (true) {
#ifdef PICO_ON_DEVICE
        tight_loop_contents();
#else
        break;
#endif
    }

    return 0;
}
