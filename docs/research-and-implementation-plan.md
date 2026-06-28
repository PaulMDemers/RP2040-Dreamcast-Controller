# RP2040/RP2350 Dreamcast Controller Emulator Plan

## Goal

Build firmware for a Raspberry Pi Pico 2 that appears to a Dreamcast as a standard main controller peripheral. The first working milestone is: console enumerates the Pico 2 as a controller, polls it with `Get Condition`, and receives valid digital buttons, analog stick, and analog trigger values.

The design should keep the Maple transport reusable so later firmware can add VMU, jump pack, keyboard, mouse, or passthrough modes without rewriting the bus layer.

## Research Summary

### Dreamcast Controller Hardware

- A standard controller is a main peripheral on one Dreamcast Maple bus.
- The controller connector has two Maple signal lines, 5 V power, ground, and a sense/ground line depending on cable/reference. The Maple signal lines are 3.3 V TTL even though the console supplies 5 V power.
- Modern Dreamcast documentation says the console does not rely on a separate sense line for normal presence detection; it sends device info requests every video frame until a main peripheral responds.
- Standard controller controls: D-pad, Start, A/B/X/Y, analog stick, and left/right analog triggers. Maple also reserves bits for C/Z/D, second D-pad, and second analog stick variants.

### Maple Bus Essentials

- Maple is a symmetrical two-wire synchronous serial protocol. The two signal lines alternate between clock and data roles.
- Nominal raw data rate is about 2 Mbps. Dreamcast host timings are around 160 ns per phase; peripherals are commonly slower.
- A packet starts with A low and four B toggles, then packet bits, an 8-bit checksum, and an end sequence.
- Packet data is grouped into 32-bit words. Bytes are transmitted MSB-first within each byte, but 4-byte groups are sent in reversed byte order at the wire level.
- The checksum/CRC byte is a bytewise XOR of all bytes in the packet.
- Frame word fields:
  - byte 3: command/response
  - byte 2: recipient address
  - byte 1: sender address
  - byte 0: payload length in 32-bit words
- Address byte uses bits 7-6 for controller port and bits 5-0 for units. Main peripheral is bit 5. Sub-peripheral bits are 0-4.
- Important commands:
  - `0x01`: device info request
  - `0x02`: extended device info request
  - `0x03`: reset
  - `0x04`: shutdown
  - `0x05`: device info response
  - `0x06`: extended device info response
  - `0x07`: acknowledge
  - `0x08`: data transfer response
  - `0x09`: get condition
  - `0xFE`: function unsupported
  - `0xFD`: unknown command
  - `0xFC`: request resend

### Controller Personality

- Controller function code is `0x00000001`.
- `Device Info` response payload is 28 words:
  - supported function mask
  - three function data words
  - area code
  - connector direction
  - product name string
  - product license string
  - standby power
  - maximum power
- Controller function data is a bitfield describing present controls. For a standard HKT-7700-style controller, set A/B/X/Y, Start, D-pad, analog X/Y, and analog L/R triggers.
- `Get Condition` request contains the controller function code. The response is command `0x08` with payload:
  - word 0: controller function code
  - word 1: left trigger, right trigger, high button byte, low button byte
  - word 2: right stick Y/X, left stick Y/X
- Digital button bits are active-low: `0` means pressed, `1` means released.
- Analog conventions:
  - triggers: `0` released, `255` fully pressed
  - stick axes: `0` negative/up/left, `128` center, `255` positive/down/right

### Existing Project Lessons

DreamPicoPort is the closest modern reference. It targets RP2040/RP2350-class chips, uses PIO state machines for Maple input and output, and uses DMA to keep PIO FIFOs fed/drained. Its structure strongly supports a similar transport architecture here.

Useful reference decisions from that project:

- Use separate PIO programs for RX and TX because the protocol is too large to comfortably fit both directions in one 32-instruction PIO program.
- Use DMA for packet TX/RX buffers.
- Check the bus is neutral before taking control.
- Start RX, wait for a host packet, parse it on the CPU, then wait the required neutral/response delay before TX.
- Treat byte order and wire-order conversion as a first-class packet API concern.

## Proposed Architecture

### Firmware Layers

1. `maple_phy`
   - PIO RX/TX programs.
   - GPIO setup for A/B Maple lines.
   - Optional direction-control pin for external transceiver hardware.
   - Open-line detection and line-release behavior.

2. `maple_transport`
   - Packet RX/TX orchestration.
   - DMA buffer ownership.
   - CRC/XOR generation and validation.
   - Frame word encode/decode.
   - Wire-order/host-order conversion.
   - Timeouts, inter-word gap handling, and stats counters.

3. `maple_device`
   - Main-peripheral address matching.
   - Dispatch commands.
   - Generate standard Maple responses and errors.
   - Track attached sub-peripheral mask, initially zero.

4. `controller_emulator`
   - Static device info.
   - Current controller condition state.
   - `Get Condition` response builder.
   - Input normalization from whichever frontend we choose.

5. `input_frontend`
   - Pluggable source for controller state.
   - Phase 1 can use USB serial/WebUSB debug commands or hardcoded test patterns.
   - Phase 2 can add USB host gamepad support, GPIO controls, or another transport.

6. `app`
   - Pico SDK startup.
   - Main loop.
   - Logging, LED status, and watchdog.

### Hardware Interface

Minimum recommended circuit:

- Power Pico 2 from console 5 V through a suitable regulator path or from USB during bench testing, with shared ground only when safe.
- Connect Maple A/B through protection/current-limiting appropriate for bidirectional 3.3 V TTL bus behavior.
- Prefer open-drain-like behavior: drive low/high only when transmitting, otherwise release to input with pullups.
- Include optional direction-enable support if using a level shifter/transceiver.
- Add series resistors and ESD protection before connecting to real console hardware.

Bench-first path:

- Validate PIO timing on a logic analyzer before attaching to Dreamcast.
- Test against another Pico acting as a Maple host simulator before using a console.

## Implementation Phases

### Phase 0: Repo and Build Skeleton

- Initialize a Pico SDK C/C++ project for Pico 2/RP2350.
- Add CMake, `pico_sdk_import.cmake`, `.gitignore`, and a host-test target.
- Add `docs/` notes for pinout, logic analyzer captures, and protocol observations.
- Add compile-time board config for Maple A/B pins, optional direction pin, and status LED.

Done when:

- Firmware builds to UF2.
- Host unit tests build and run on macOS/Linux.

### Phase 1: Pure Packet Model and Tests

- Implement Maple frame encode/decode.
- Implement packet byte-order conversion.
- Implement checksum generation/validation.
- Implement controller condition packing.
- Implement device info payload packing.
- Add host tests with known packet vectors.

Done when:

- Unit tests prove stable packet packing for device info and get-condition responses.
- Tests cover active-low button behavior and analog center/default values.

Status: implemented with host tests.

### Phase 2: PIO Maple Transport

- Port or write TX PIO:
  - start sequence
  - alternating clock/data bits
  - CRC byte
  - end sequence
- Port or write RX PIO:
  - detect start sequence
  - sample alternating bits
  - detect end or stop based on expected length/CRC
- Add DMA buffers for RX and TX.
- Add line idle check and release behavior.
- Add timing constants for Pico 2 clock.

Done when:

- Logic analyzer shows valid start/data/end sequences.
- A software Maple host simulator can send packets that firmware receives and parses.

Status: packet-to-wire bitstream, wire decode, PIO TX buffer layout, and controller session dispatch are implemented and host-tested. Actual Pico PIO programs and hardware timing validation remain.

### Phase 3: Main Peripheral State Machine

- Start in RX mode waiting for host packets.
- Accept packets addressed to the main peripheral bit for the active port.
- Respond to:
  - `Device Info Request` with `Device Info`
  - `Extended Device Info Request` with either extended info or normal info plus version string
  - `Get Condition` for controller function with `Data Transfer`
  - `Reset` and `Shutdown` with `ACK`
  - unsupported functions with `Function Code Not Supported`
  - unknown commands with `Unknown Command`
- Enforce one response packet per request.
- Add response delay after host releases bus.

Done when:

- A host-side Maple test sends request frames and receives valid response frames.
- Invalid CRC and unsupported command cases are handled predictably.

### Phase 4: Controller Input Frontend

- Define `ControllerState`:
  - 16-bit active-low button mask internally represented as logical booleans or normalized mask.
  - left stick X/Y.
  - optional right stick X/Y defaulting to center.
  - left/right trigger values.
- Add a simple debug frontend first:
  - USB serial commands to set buttons/axes.
  - optional auto-demo mode for testing.
- Add chosen real input path:
  - USB host HID gamepad on Pico 2, or
  - GPIO matrix/direct controls, or
  - custom serial bridge from PC.

Done when:

- Dreamcast/Flycast-side poll results change as input changes.

Status: first debug text command parser is implemented and host-tested. USB stdio integration remains.

### Phase 5: Console Bring-Up

- Connect through protected hardware.
- Observe console enumeration:
  - host sends repeated device info requests.
  - Pico responds with device info.
  - host starts polling `Get Condition`.
- Test in Dreamcast BIOS and a small set of games.
- Tune timing if necessary:
  - response delay
  - bit time
  - inter-word gap tolerance
  - bus idle check

Done when:

- Console detects controller from cold boot.
- Buttons, stick, and triggers work in BIOS/game menus.

### Phase 6: Hardening and Extensions

- Add persistent config.
- Add multi-controller support if pins/PIO/DMA resources allow.
- Add VMU identity and minimal storage later if needed.
- Add jump pack/vibration support via `Set Condition` for function `0x00000100`.
- Add structured diagnostics over USB.
- Add capture-based regression tests from real console packets.

## Open Questions

- What is the first real input source: USB gamepad host, direct GPIO controls, serial bridge, or something else?
- Will the Pico 2 be powered by the Dreamcast controller port, USB, or both?
- Do we want a standard controller only, or should the first firmware advertise VMU/jump-pack sub-peripheral bits?
- Is the target a real Dreamcast first, Flycast/maple adapter first, or both?
- Do we want to reuse MIT-licensed DreamPicoPort Maple PIO code directly, or write a clean-room transport inspired by the protocol docs?

## Main Risks

- Electrical safety: incorrect bus drive behavior can fight the console or attached peripherals.
- Timing: Maple is fast enough that CPU bit-banging is risky; PIO plus DMA is the right path.
- Byte order: frame words, payload words, and wire byte order are easy to mix up.
- Controller compatibility: some games may depend on exact device info strings, function data, or response timing.
- Debug logging: UART/USB logging inside timing-critical paths can break communication.

## Suggested First Milestone

Implement a single-port, no-sub-peripheral standard controller emulator with a USB serial debug input frontend. This gives the shortest route to a console-recognized controller while keeping hardware, input mapping, VMU, and vibration complexity out of the first bring-up.

## Reference Links

- Dreamcast.wiki Maple bus: https://dreamcast.wiki/Maple_bus
- Dreamcast.wiki Controller peripheral: https://dreamcast.wiki/Controller_peripheral
- Marcus Comstedt Maple Bus: http://mc.pp.se/dc/maplebus.html
- Marcus Comstedt Maple Wire Protocol: http://mc.pp.se/dc/maplewire.html
- Marcus Comstedt Controllers: http://mc.pp.se/dc/controller.html
- DreamPicoPort / DreamcastControllerUsbPico: https://github.com/OrangeFox86/DreamPicoPort
- DreamPicoPort Maple PIO RX/TX: https://github.com/OrangeFox86/DreamPicoPort/tree/main/src/hal/MapleBus
