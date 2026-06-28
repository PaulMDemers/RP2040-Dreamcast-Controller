# RP2040 Dreamcast Controller Emulator

Firmware and protocol tooling for emulating a Sega Dreamcast controller with a Raspberry Pi Pico 2 / RP2350-class board.

The project is in early bring-up. The host-testable Maple packet, controller personality, wire bitstream, PIO buffer, session dispatch, and debug input parser are implemented. The actual Pico PIO RX/TX hardware layer is the next major milestone.

## Status

Implemented:

- Maple frame encode/decode
- host-order and wire-order word conversion
- Maple XOR checksum generation and validation
- standard controller device-info payload
- standard controller condition response packing
- controller command dispatcher
- raw host-bitstream session handling
- PIO/DMA-friendly TX buffer layout
- debug input parser for commands such as `press a`, `release start`, and `set lx 200`
- no-dependency host tests and packet simulator

Not implemented yet:

- Pico PIO RX/TX programs wired into firmware
- DMA-backed hardware transport
- USB stdio command loop
- USB HID host input
- VMU or jump-pack sub-peripheral emulation
- real-console timing validation

## Safety

Proceed carefully when connecting anything to real Dreamcast hardware.

The Dreamcast controller port provides 5 V power, while Maple signal lines are documented as 3.3 V TTL. Use current limiting, ESD protection, and a protected interface before connecting to a console. Verify bus behavior with a logic analyzer first, and make sure the Pico releases the Maple lines when idle or reset.

This project is not affiliated with, endorsed by, or sponsored by Sega.

## Repository Layout

- `include/dreamcast/`: public core headers
- `src/`: core implementation
- `app/`: Pico firmware entry point skeleton
- `tests/`: host-side test runner
- `tools/`: host-side packet simulator
- `docs/`: protocol research, build notes, and bring-up notes

## Host Tests

The core code does not require the Pico SDK. On macOS/Linux with a C++17 compiler:

```sh
make test
```

## Packet Simulator

To print sample Dreamcast request/controller response packets:

```sh
make sim
```

Packet fixtures are documented in `docs/bringup/packet-fixtures.md`.

## Pico 2 Firmware Build

Install the Raspberry Pi Pico SDK, CMake, and an ARM embedded toolchain, then:

```sh
export PICO_SDK_PATH=/path/to/pico-sdk
cmake -S . -B build -DPICO_BOARD=pico2
cmake --build build
```

The firmware target is `dreamcast_controller`. At the moment this target is a skeleton while the Maple hardware transport is being implemented.

## Documentation

- `docs/research-and-implementation-plan.md`: protocol research and staged implementation plan
- `docs/building.md`: build and module notes
- `docs/bringup/hardware-notes.md`: electrical and bench bring-up checklist
- `docs/bringup/packet-fixtures.md`: known packet values for tests and logic analyzer work

## License

MIT. See `LICENSE`.

