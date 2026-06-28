# Contributing

Thanks for helping improve the Dreamcast controller emulator.

## Development Loop

Run host tests before submitting changes:

```sh
make test
```

Run the packet simulator when changing Maple packet layout:

```sh
make sim
```

If simulator output intentionally changes, update `docs/bringup/packet-fixtures.md`.

## Hardware Changes

Document hardware-facing changes in `docs/bringup/`. Include:

- target board and pins
- wiring assumptions
- logic analyzer captures or expected packet values
- whether the change was tested on a bench simulator or real console

## Code Style

- Use C++17 for portable core code.
- Keep protocol packing code host-testable and independent from Pico SDK headers.
- Keep Pico SDK and PIO-specific code behind firmware-facing modules.
- Prefer small focused tests for packet, byte-order, checksum, and state-machine behavior.

