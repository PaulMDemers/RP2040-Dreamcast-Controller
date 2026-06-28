cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if(DEFINED ENV{PICO_SDK_PATH})
    include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
else()
    message(FATAL_ERROR "PICO_SDK_PATH is not set. Install the Pico SDK and export PICO_SDK_PATH before configuring firmware.")
endif()

project(rp2040_dreamcast_controller C CXX ASM)

pico_sdk_init()

add_library(dreamcast_core
    src/maple_packet.cpp
    src/maple_wire.cpp
    src/maple_pio_buffer.cpp
    src/maple_session.cpp
    src/dreamcast_controller.cpp
    src/controller_input_parser.cpp
    src/controller_device.cpp
)
target_include_directories(dreamcast_core PUBLIC include)

add_executable(dreamcast_controller
    app/main.cpp
)
target_link_libraries(dreamcast_controller
    dreamcast_core
    pico_stdlib
)

pico_enable_stdio_usb(dreamcast_controller 1)
pico_enable_stdio_uart(dreamcast_controller 0)
pico_add_extra_outputs(dreamcast_controller)
