# Building

## Host Tests

The core Maple packet and controller code has no Pico SDK dependency. Run:

```sh
make test
```

This builds `build-host/dreamcast_core_tests` with the system C++ compiler.

## Host Packet Simulator

Run:

```sh
make sim
```

This prints a device-info exchange and a get-condition exchange for a port-A controller. Keep `docs/bringup/packet-fixtures.md` in sync when changing packet layout.

## Current Core Modules

- `maple_packet`: Maple frame/payload words, byte order, checksum.
- `maple_wire`: MSB-first serial bitstream encode/decode, including checksum validation.
- `maple_pio_buffer`: PIO/DMA-friendly TX buffer with bit count, wire-order words, and CRC high byte.
- `controller_device`: standard controller identity, state, and Maple command dispatch.
- `maple_session`: raw host bitstream to controller response/PIO TX buffer.
- `controller_input_parser`: debug text commands for updating controller state.
- `pico/maple_phy`: Pico SDK-facing Maple PIO TX wrapper and pin management scaffold.

## Debug Input Commands

The parser accepts simple line-oriented commands:

```text
press a
release start
set lx 128
set ly 128
set lt 0
set rt 255
neutral
```

Controls: `a`, `b`, `x`, `y`, `c`, `z`, `d`, `start`, `up`, `down`, `left`, `right`, `lx`, `ly`, `rx`, `ry`, `lt`, `rt`.

## Pico Transport Status

The firmware target now includes:

- Maple TX PIO program generation
- Maple RX PIO program generation
- `PicoMaplePhy` pin setup and line release
- blocking TX path for `MaplePioTxBuffer`
- USB stdio command parsing into `ControllerState`

Remaining hardware work:

- DMA or FIFO-backed RX collection
- feeding RX packets into `ControllerMapleSession`
- scheduling response delay after host packet end
- logic analyzer timing validation

## Pico 2 Firmware

Install the Raspberry Pi Pico SDK, CMake, and an ARM embedded toolchain, then configure with:

```sh
export PICO_SDK_PATH=/path/to/pico-sdk
cmake -S . -B build -DPICO_BOARD=pico2
cmake --build build
```

The firmware target is `dreamcast_controller`. The current firmware entry point is a skeleton while the hardware Maple PIO layer is being brought up.

Known-good local setup:

```sh
brew install cmake
brew install armmbed/formulae/arm-none-eabi-gcc
export PICO_SDK_PATH=/Users/pauldemers/pico/pico-sdk
cmake -S . -B build -DPICO_BOARD=pico2
cmake --build build
```

Avoid Homebrew's core `arm-none-eabi-gcc` formula for this project: that package may be built `--without-headers`, which causes firmware builds to fail on standard C++ headers such as `<cstddef>`. The ArmMbed formula installs the embedded runtime headers needed by Pico SDK C++ builds.

If CMake warns that TinyUSB submodules are not initialized, USB stdio may be unavailable until the SDK submodules are fetched:

```sh
cd "$PICO_SDK_PATH"
git submodule update --init
```

With the SDK at `/Users/pauldemers/pico/pico-sdk`, the Pico 2 build currently produces `build/dreamcast_controller.uf2`.
