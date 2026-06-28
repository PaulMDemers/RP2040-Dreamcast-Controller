# Hardware Bring-Up Notes

## Initial Target

Single Pico 2 as a Maple main peripheral on Dreamcast port A.

## Signal Assumptions

- Maple A/B are 3.3 V TTL signal lines.
- The Dreamcast controller port supplies 5 V power.
- Firmware should release both Maple lines when idle.
- Before attaching to a Dreamcast, verify with a logic analyzer that the Pico never drives against another transmitter.

## Suggested Bench Order

1. Run `make test` and `make sim`.
2. Build firmware once Pico SDK/CMake/toolchain are installed.
3. Scope TX-only generated packets into a high impedance analyzer.
4. Use a second Pico or logic-level generator as a host simulator.
5. Only then connect to a real Dreamcast through the protected interface.

## Protection Checklist

- Common ground is intentional and solid.
- A/B have current limiting.
- A/B have ESD protection if leaving a PCB or enclosure.
- Any level shifter/transceiver direction pin defaults to receive/high impedance during reset.
- USB-powered bench mode cannot backfeed the Dreamcast 5 V rail.

